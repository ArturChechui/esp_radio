#include "EventTask.hpp"

#include <esp_log.h>

#include <new>

#include "IEventHandler.hpp"

namespace core {
namespace {
constexpr uint32_t QueueSendTimeoutMs = 100U;      // ms
constexpr uint32_t QueueReceiveTimeoutMs = 1000U;  // ms

}  // namespace

EventTask::EventTask(const char* name, const uint8_t& queueSize, const uint16_t& stackSize,
                     const uint8_t& priority)
    : mName(name),
      mQueueSize(queueSize),
      mStackSize(stackSize),
      mPriority(priority),
      mEventQueue(nullptr),
      mTaskHandle(nullptr),
      mIsRunning(false),
      mHandler(nullptr) {
    ESP_LOGI(mName, "EventTask created");
}

EventTask::~EventTask() {
    stop();
}

bool EventTask::init(common::IEventHandler& handler) {
    mHandler = &handler;
    mIsRunning = true;
    if (mEventQueue != nullptr) {
        ESP_LOGW(mName, "Already initialized");
        return false;
    }

    // IMPORTANT:
    // FreeRTOS queues copy elements with memcpy. common::AppEvent is a std::variant and
    // currently contains non-trivial types (e.g. std::string via WifiStateChangedEvent).
    // Copying such objects with memcpy causes memory corruption and random crashes.
    //
    // To make this safe, we pass pointers through the queue and allocate events on the heap.
    mEventQueue = xQueueCreate(mQueueSize, sizeof(common::AppEvent*));
    if (!mEventQueue) {
        ESP_LOGE(mName, "Failed to create event queue");
        return false;
    }

    mStoppedSem = xSemaphoreCreateBinary();
    if (!mStoppedSem) {
        return false;
    }

    // Create task
    ESP_LOGI(mName, "Creating task");
    const BaseType_t res =
        xTaskCreate(EventTask::taskEntry, mName, mStackSize, this, mPriority, &mTaskHandle);

    if (res != pdPASS) {
        ESP_LOGE(mName, "Failed to create task");
        vQueueDelete(mEventQueue);
        mEventQueue = nullptr;
        return false;
    }

    ESP_LOGI(mName, "EventTask initialized (queue=%u, priority=%u)", mQueueSize, mPriority);
    return true;
}

bool EventTask::post(const common::AppEvent& event) {
    if (!mEventQueue) {
        ESP_LOGE(mName, "Queue not initialized");
        return false;
    }

    // Allocate an event copy on heap (safe for std::variant/std::string)
    auto* heapEvent = new (std::nothrow) common::AppEvent(event);
    if (!heapEvent) {
        ESP_LOGE(mName, "Failed to allocate event");
        return false;
    }

    const BaseType_t res = xQueueSend(mEventQueue, &heapEvent, pdMS_TO_TICKS(QueueSendTimeoutMs));
    if (res != pdPASS) {
        ESP_LOGW(mName, "Failed to post event");
        delete heapEvent;
        return false;
    }

    return true;
}

void EventTask::stop() {
    if (!mTaskHandle) {
        return;
    }

    mIsRunning = false;

    common::AppEvent* shutdownMarker = nullptr;
    (void)xQueueSend(mEventQueue, &shutdownMarker, 0);  // wake the task

    (void)xSemaphoreTake(mStoppedSem, pdMS_TO_TICKS(2000));

    vQueueDelete(mEventQueue);
    mEventQueue = nullptr;

    vSemaphoreDelete(mStoppedSem);
    mStoppedSem = nullptr;

    mTaskHandle = nullptr;

    ESP_LOGI(mName, "EventTask stopped");
}

void EventTask::taskEntry(void* pvParameters) {
    auto* pThis = static_cast<EventTask*>(pvParameters);
    if (pThis == nullptr) {
        ESP_LOGE("EventTask", "Invalid task parameter");
        vTaskDelete(nullptr);
        return;
    }

    pThis->runLoop();

    vTaskDelete(nullptr);
}

void EventTask::runLoop() {
    ESP_LOGI(mName, "Event loop started");

    common::AppEvent* eventPtr = nullptr;
    while (mIsRunning) {
        const BaseType_t result =
            xQueueReceive(mEventQueue, &eventPtr, pdMS_TO_TICKS(QueueReceiveTimeoutMs));
        if (result == pdTRUE) {
            if (eventPtr == nullptr) {
                break;
            }
            if (mHandler) {
                mHandler->onEvent(*eventPtr);
            }

            delete eventPtr;
            eventPtr = nullptr;
        }
    }

    // Drain leftovers safely
    while (xQueueReceive(mEventQueue, &eventPtr, 0) == pdTRUE) {
        delete eventPtr;
    }

    ESP_LOGI(mName, "Event loop exited");
    xSemaphoreGive(mStoppedSem);
    vTaskDelete(nullptr);
}

}  // namespace core
