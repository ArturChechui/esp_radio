#include "EventTask.hpp"

#include <esp_log.h>

#include <new>

#include "IEventHandler.hpp"
#include "IStopToken.hpp"
#include "ITaskRunner.hpp"
#include "Types.hpp"

namespace common {
namespace {
constexpr uint32_t QueueTimeoutMs = 100U;
constexpr uint8_t QueueSize = 10U;

constexpr uint16_t StackSize = 8192U;
constexpr uint8_t TaskCore = 0U;
constexpr uint8_t TaskPriority = 6U;

constexpr const char* Tag = "EventTask";
}  // namespace

// TODO: change the AppEvent approach?
EventTask::EventTask(const char* taskName, ITaskRunner& taskRunner)
    : mTaskParams(),
      mTaskHandle(),
      mTaskRunner(taskRunner),
      mEventQueue(nullptr),
      mIsRunning(false),
      mHandler(nullptr) {
    ESP_LOGI(Tag, "[%s] EventTask created", mTaskParams.name);
    mTaskParams.core = TaskCore;
    mTaskParams.name = taskName;
    mTaskParams.priority = TaskPriority;
}

EventTask::~EventTask() {
    ESP_LOGI(Tag, "[%s] Destroying EventTask", mTaskParams.name);

    mIsRunning = false;
    if (mTaskHandle.isValid()) {
        mTaskRunner.stop(mTaskHandle, 2000U);
    }

    // TODO: make a leak if stop failed, it is safer
    if (mEventQueue != nullptr) {
        // Drain leftovers safely
        AppEvent* eventPtr = nullptr;
        while (xQueueReceive(mEventQueue, &eventPtr, 0) == pdTRUE) {
            delete eventPtr;
            eventPtr = nullptr;
        }
        vQueueDelete(mEventQueue);
        mEventQueue = nullptr;
    }
}

bool EventTask::init() {
    if (mEventQueue != nullptr) {
        ESP_LOGW(Tag, "[%s] Already initialized", mTaskParams.name);
        return false;
    }

    mEventQueue = xQueueCreate(QueueSize, sizeof(AppEvent*));
    if (!mEventQueue) {
        ESP_LOGE(Tag, "[%s] Failed to create queue", mTaskParams.name);
        return false;
    }
    ESP_LOGI(Tag, "[%s] Created queue (length=%u, itemSize=%u)", mTaskParams.name, QueueSize,
             sizeof(AppEvent*));

    ESP_LOGI(Tag, "[%s] EventTask initialized", mTaskParams.name);
    return true;
}

bool EventTask::run(IEventHandler& handler) {
    mHandler = &handler;
    mIsRunning = true;

    mTaskHandle = mTaskRunner.start(mTaskParams, StackSize, &EventTask::processStepFn, this);
    if (!mTaskHandle.isValid()) {
        ESP_LOGE(Tag, "[%s] Failed to create a task", mTaskParams.name);
        AppEvent* eventPtr = nullptr;
        while (xQueueReceive(mEventQueue, &eventPtr, 0) == pdTRUE) {
            delete eventPtr;
            eventPtr = nullptr;
        }
        vQueueDelete(mEventQueue);
        mEventQueue = nullptr;
        return false;
    }
    ESP_LOGI(Tag, "[%s] Created task (core=%u, prio=%u, stackSize=%u)", mTaskParams.name,
             mTaskParams.core, mTaskParams.priority, StackSize);

    return true;
}

bool EventTask::post(const AppEvent& event) {
    if (!mEventQueue) {
        ESP_LOGE(Tag, "[%s] Queue not initialized", mTaskParams.name);
        return false;
    }

    // TODO: come up with another solution without new
    // Allocate an event copy on heap (safe for std::variant/std::string)
    auto* heapEvent = new (std::nothrow) AppEvent(event);
    if (!heapEvent) {
        ESP_LOGE(Tag, "[%s] Failed to allocate event", mTaskParams.name);
        return false;
    }

    const BaseType_t res = xQueueSend(mEventQueue, &heapEvent, pdMS_TO_TICKS(QueueTimeoutMs));
    if (res != pdPASS) {
        ESP_LOGW(Tag, "[%s] Failed to post event", mTaskParams.name);
        delete heapEvent;
        return false;
    }

    return true;
}

StepResult EventTask::processStepFn(void* arg, IStopToken& token) {
    auto* self = static_cast<EventTask*>(arg);
    if (!self) {
        return {.action = StepAction::Error};
    }

    return self->processStep(token);
}

StepResult EventTask::processStep(IStopToken& token) {
    if (token.stopRequested() || !mIsRunning) {
        return {.action = StepAction::Done};
    }

    AppEvent* eventPtr = nullptr;
    const auto result = xQueueReceive(mEventQueue, &eventPtr, pdMS_TO_TICKS(QueueTimeoutMs));
    if (result == pdTRUE) {
        if (eventPtr != nullptr && mHandler) {
            mHandler->onEvent(*eventPtr);
        }

        delete eventPtr;
        eventPtr = nullptr;
    }

    return {.action = StepAction::Continue};
}

}  // namespace common
