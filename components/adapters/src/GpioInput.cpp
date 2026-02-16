#include "GpioInput.hpp"

#include <driver/gpio.h>
#include <esp_log.h>

#include "BoardConfig.hpp"
#include "Queue.hpp"

namespace adapters {
namespace {
constexpr const char* Tag = "GpioInput";
}  // namespace

GpioInput* GpioInput::sInstance = nullptr;

GpioInput::GpioInput(common::Queue<uint32_t>& queue) : mQueue(queue), mIsInitialized(false) {
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
    uint64_t pinBtnMask = 0;
    for (auto btn : common::ButtonGpios) {
        pinBtnMask |= (1ULL << btn);
    }

    gpio_config_t btnCfg = {};
    btnCfg.pin_bit_mask = pinBtnMask;
    btnCfg.mode = GPIO_MODE_INPUT;
    btnCfg.pull_up_en = GPIO_PULLUP_ENABLE;
    btnCfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    btnCfg.intr_type = GPIO_INTR_NEGEDGE;
    esp_err_t cfgRet = gpio_config(&btnCfg);
    if (cfgRet != ESP_OK) {
        ESP_LOGE(Tag, "gpio_config failed: %s", esp_err_to_name(cfgRet));
        return false;
    }

    gpio_config_t encCfg{};
    encCfg.intr_type = GPIO_INTR_ANYEDGE;
    encCfg.mode = GPIO_MODE_INPUT;
    encCfg.pin_bit_mask = (1ULL << common::EncS1Gpio) | (1ULL << common::EncS2Gpio);
    encCfg.pull_up_en = GPIO_PULLUP_ENABLE;
    encCfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfgRet = gpio_config(&encCfg);
    if (cfgRet != ESP_OK) {
        ESP_LOGE(Tag, "gpio_config failed: %s", esp_err_to_name(cfgRet));
        return false;
    }

    // Install ISR service
    esp_err_t isrRet = gpio_install_isr_service(0);
    if (isrRet != ESP_OK && isrRet != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(Tag, "gpio_install_isr_service failed: %s", esp_err_to_name(isrRet));
        return false;
    }

    // Hook ISR handlers
    for (auto btn : common::ButtonGpios) {
        isrRet = gpio_isr_handler_add(static_cast<gpio_num_t>(btn), gpioIsrHandler,
                                      reinterpret_cast<void*>(btn));
        if (isrRet != ESP_OK) {
            ESP_LOGE(Tag, "gpio_isr_handler_add failed for GPIO %llu: %s", btn,
                     esp_err_to_name(isrRet));
            deinit();
            return false;
        }
    }

    // add handlers for EncS1Gpio and EncS2Gpio
    isrRet = gpio_isr_handler_add(static_cast<gpio_num_t>(common::EncS1Gpio), &gpioIsrHandler,
                                  reinterpret_cast<void*>(common::EncS1Gpio));
    if (isrRet != ESP_OK) {
        ESP_LOGE(Tag, "gpio_isr_handler_add failed for GPIO %llu: %s", common::EncS1Gpio,
                 esp_err_to_name(isrRet));
        deinit();
        return false;
    }

    isrRet = gpio_isr_handler_add(static_cast<gpio_num_t>(common::EncS2Gpio), &gpioIsrHandler,
                                  reinterpret_cast<void*>(common::EncS2Gpio));
    if (isrRet != ESP_OK) {
        ESP_LOGE(Tag, "gpio_isr_handler_add failed for GPIO %llu: %s", common::EncS2Gpio,
                 esp_err_to_name(isrRet));
        deinit();
        return false;
    }

    mIsInitialized = true;
    ESP_LOGI(Tag, "GpioInput initialized (%u buttons)",
             static_cast<unsigned>(common::ButtonGpios.size()));

    return true;
}

void GpioInput::deinit() {
    if (!mIsInitialized) {
        return;
    }

    // Remove ISR handlers
    for (auto btn : common::ButtonGpios) {
        gpio_isr_handler_remove(static_cast<gpio_num_t>(btn));
    }

    gpio_isr_handler_remove(static_cast<gpio_num_t>(common::EncS1Gpio));
    gpio_isr_handler_remove(static_cast<gpio_num_t>(common::EncS2Gpio));
    // TODO: uninstall ISR, no one else uses it, right?

    mIsInitialized = false;
    sInstance = nullptr;

    ESP_LOGI(Tag, "GpioInput deinitialized");
}

int GpioInput::getLevel(const uint32_t gpioNum) {
    return gpio_get_level((gpio_num_t)gpioNum);
}

void GpioInput::gpioIsrHandler(void* arg) {
    auto* inst = sInstance;
    if (!inst) {
        // TODO: add a check for the queue
        return;
    }

    const uint32_t gpioNum = (uint32_t)(uintptr_t)arg;
    inst->mQueue.pushFromIsr(gpioNum);
}

}  // namespace adapters
