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

    if (mEventQueue) {
        vQueueDelete(mEventQueue);
        mEventQueue = nullptr;
    }
}

bool EventTask::init() {
    if (mEventQueue) {
        ESP_LOGW(Tag, "[%s] Already initialized", mTaskParams.name);
        return false;
    }

    mEventQueue = xQueueCreate(QueueSize, sizeof(AppEvent));
    if (!mEventQueue) {
        ESP_LOGE(Tag, "[%s] Failed to create queue", mTaskParams.name);
        return false;
    }
    ESP_LOGI(Tag, "[%s] Created queue (length=%u, itemSize=%u)", mTaskParams.name, QueueSize,
             sizeof(AppEvent));

    ESP_LOGI(Tag, "[%s] EventTask initialized", mTaskParams.name);
    return true;
}

bool EventTask::run(IEventHandler& handler) {
    mHandler = &handler;
    mIsRunning = true;

    mTaskHandle = mTaskRunner.start(mTaskParams, StackSize, &EventTask::processStepFn, this);
    if (!mTaskHandle.isValid()) {
        ESP_LOGE(Tag, "[%s] Failed to create a task", mTaskParams.name);
        if (mEventQueue) {
            vQueueDelete(mEventQueue);
            mEventQueue = nullptr;
        }
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

    const BaseType_t res = xQueueSend(mEventQueue, &event, pdMS_TO_TICKS(QueueTimeoutMs));
    if (res != pdPASS) {
        ESP_LOGW(Tag, "[%s] Failed to post event", mTaskParams.name);
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

    AppEvent event;
    const auto result = xQueueReceive(mEventQueue, &event, pdMS_TO_TICKS(QueueTimeoutMs));
    if ((result == pdTRUE) && mHandler) {
        mHandler->onEvent(event);
    }

    return {.action = StepAction::Continue};
}

}  // namespace common
