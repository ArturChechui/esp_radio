#include "GpioInput.hpp"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "BoardConfig.hpp"
#include "SemaphoreGuard.hpp"

namespace adapters {

namespace {
constexpr const char* Tag = "GpioInput";

// Debounce time in task context
constexpr uint32_t DebounceDelayMs = 20U;
constexpr TickType_t DebounceTicks = pdMS_TO_TICKS(DebounceDelayMs);

// ISR queue depth (raw GPIO numbers)
constexpr uint8_t IsrQueueSize = 16U;

// Task settings
constexpr uint16_t TaskStackWords = 6144;  // stack in words for xTaskCreate
constexpr UBaseType_t TaskPriority = 7U;

// We treat buttons as active-low with internal pull-up.
constexpr int ButtonPressedLevel = 0;

constexpr std::array<uint32_t, 3> ButtonGpios = {
    common::BUTTON_PLAY_STOP_GPIO,
    common::BUTTON_UP_GPIO,
    common::BUTTON_DOWN_GPIO,
};

constexpr bool isValidGpio(uint32_t gpio) {
    for (auto g : ButtonGpios) {
        if (g == gpio) {
            return true;
        }
    }
    return false;
}

}  // namespace

GpioInput* GpioInput::sInstance = nullptr;

GpioInput::GpioInput()
    : mInputDataCb(nullptr),
      mCallbackMutex(),
      mIsInitialized(false),
      mIsrQueue(nullptr),
      mTaskHandle(nullptr),
      mLastPressTick{0, 0, 0} {
    ESP_LOGI(Tag, "GpioInput created");
}

GpioInput::~GpioInput() {
    deinit();
}

bool GpioInput::init() {
    if (mIsInitialized) {
        return true;
    }

    sInstance = this;

    // Build bitmask for all buttons
    uint64_t pinMask = 0;
    for (auto btn : ButtonGpios) {
        pinMask |= (1ULL << static_cast<uint64_t>(btn));
    }

    gpio_config_t gpioConfig = {};
    gpioConfig.pin_bit_mask = pinMask;
    gpioConfig.mode = GPIO_MODE_INPUT;
    gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
    gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpioConfig.intr_type = GPIO_INTR_NEGEDGE;  // press = falling edge

    const esp_err_t cfgRet = gpio_config(&gpioConfig);
    if (cfgRet != ESP_OK) {
        ESP_LOGE(Tag, "gpio_config failed: %s", esp_err_to_name(cfgRet));
        return false;
    }

    // Create queue for ISR -> task
    mIsrQueue = xQueueCreate(IsrQueueSize, sizeof(uint32_t));
    if (!mIsrQueue) {
        ESP_LOGE(Tag, "Failed to create ISR queue");
        return false;
    }

    // Install ISR service (ignore if already installed)
    esp_err_t isrRet = gpio_install_isr_service(0);
    if (isrRet != ESP_OK && isrRet != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(Tag, "gpio_install_isr_service failed: %s", esp_err_to_name(isrRet));
        vQueueDelete(mIsrQueue);
        mIsrQueue = nullptr;
        return false;
    }

    // Hook ISR handlers
    for (auto btn : ButtonGpios) {
        isrRet = gpio_isr_handler_add(static_cast<gpio_num_t>(btn), gpioIsrHandler,
                                      reinterpret_cast<void*>(btn));
        if (isrRet != ESP_OK) {
            ESP_LOGE(Tag, "gpio_isr_handler_add failed for GPIO %lu: %s", btn,
                     esp_err_to_name(isrRet));
            deinit();
            return false;
        }
    }

    // Create a task that does debounce + callback dispatch.
    const BaseType_t taskRet =
        xTaskCreate(gpioTaskFn, "GpioInputTask", TaskStackWords, this, TaskPriority, &mTaskHandle);
    if (taskRet != pdPASS) {
        ESP_LOGE(Tag, "Failed to create GPIO task");
        deinit();
        return false;
    }

    mIsInitialized = true;
    ESP_LOGI(Tag, "GpioInput initialized (%u buttons)", static_cast<unsigned>(ButtonGpios.size()));
    return true;
}

void GpioInput::deinit() {
    if (!mIsInitialized) {
        return;
    }

    // Remove ISR handlers
    for (auto btn : ButtonGpios) {
        gpio_isr_handler_remove(static_cast<gpio_num_t>(btn));
    }

    // Stop task
    if (mTaskHandle) {
        vTaskDelete(mTaskHandle);
        mTaskHandle = nullptr;
    }

    // Delete queue
    if (mIsrQueue) {
        vQueueDelete(mIsrQueue);
        mIsrQueue = nullptr;
    }

    // Do NOT uninstall ISR service globally here.
    // Other components might also use it, and uninstalling causes surprises.

    {
        common::SemaphoreGuard guard(mCallbackMutex);
        mInputDataCb = nullptr;
    }

    mIsInitialized = false;
    sInstance = nullptr;
    ESP_LOGI(Tag, "GpioInput deinitialized");
}

void GpioInput::setInputCallback(common::GpioInputDataCallback cb) {
    common::SemaphoreGuard guard(mCallbackMutex);
    mInputDataCb = std::move(cb);
}

void GpioInput::gpioIsrHandler(void* arg) {
    auto* inst = sInstance;
    if (!inst || !inst->mIsrQueue) {
        return;
    }

    const uint32_t gpioNum = (uint32_t)(uintptr_t)arg;

    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(inst->mIsrQueue, &gpioNum, &hpw);

    if (hpw) {
        portYIELD_FROM_ISR();
    }
}
void GpioInput::gpioTaskFn(void* arg) {
    auto* self = static_cast<GpioInput*>(arg);
    if (!self) {
        vTaskDelete(nullptr);
        return;
    }

    self->gpioTaskLoop();
    vTaskDelete(nullptr);
}

void GpioInput::gpioTaskLoop() {
    uint32_t gpioNum = 0;

    static uint32_t drop_unknown_gpio = 0;
    static uint32_t drop_lockout = 0;
    static uint32_t drop_level_mismatch = 0;
    static uint32_t sent_ok = 0;

    static TickType_t lastPrint = 0;

    constexpr TickType_t ConfirmDelay = pdMS_TO_TICKS(20);
    constexpr TickType_t LockoutDelay = pdMS_TO_TICKS(200);

    while (true) {
        if (xQueueReceive(mIsrQueue, &gpioNum, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        const TickType_t now = xTaskGetTickCount();

        int idx = -1;
        for (size_t i = 0; i < ButtonGpios.size(); ++i) {
            if (ButtonGpios[i] == gpioNum) {
                idx = (int)i;
                break;
            }
        }

        if (idx < 0) {
            drop_unknown_gpio++;
            continue;
        }

        // Lockout debounce (prevents multi-trigger)
        if ((now - mLastPressTick[(size_t)idx]) < LockoutDelay) {
            drop_lockout++;
            continue;
        }

        // Confirm press after short delay
        const int level0 = gpio_get_level((gpio_num_t)gpioNum);
        vTaskDelay(ConfirmDelay);
        const int level1 = gpio_get_level((gpio_num_t)gpioNum);

        if (!(level0 == ButtonPressedLevel && level1 == ButtonPressedLevel)) {
            drop_level_mismatch++;
            continue;
        }

        mLastPressTick[(size_t)idx] = now;
        dispatchButtonPress(gpioNum);
        sent_ok++;

        if ((now - lastPrint) > pdMS_TO_TICKS(1000)) {
            lastPrint = now;
            auto words = uxTaskGetStackHighWaterMark(nullptr);
            ESP_LOGI("GpioInputTask",
                     "sent=%lu drop:unknown=%lu lockout=%lu mismatch=%lu stack_free=%luB",
                     (unsigned long)sent_ok, (unsigned long)drop_unknown_gpio,
                     (unsigned long)drop_lockout, (unsigned long)drop_level_mismatch,
                     (unsigned long)(words * sizeof(StackType_t)));
        }
    }
}

void GpioInput::dispatchButtonPress(const uint32_t gpioNum) {
    common::GpioInputDataCallback cb;
    {
        common::SemaphoreGuard guard(mCallbackMutex);
        cb = mInputDataCb;
    }

    if (!cb) {
        ESP_LOGW("GpioInput", "Button %lu ignored: callback not set", (unsigned long)gpioNum);
        return;
    }

    ESP_LOGI("GpioInput", "Button %lu -> sending cb()", (unsigned long)gpioNum);

    const common::GpioInputData data = {
        .type = common::GpioInputType::ButtonPressed,
        .buttonId = gpioNum,
        .timestampMs = static_cast<uint64_t>(pdTICKS_TO_MS(xTaskGetTickCount())),
    };

    cb(data);
    ESP_LOGI("GpioInput", "Button %lu -> cb() returned", (unsigned long)gpioNum);
}

}  // namespace adapters
