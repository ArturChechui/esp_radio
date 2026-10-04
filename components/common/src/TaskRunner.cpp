#include "TaskRunner.hpp"

#include <esp_log.h>

#include <cstring>

#include "Helper.hpp"
#include "StopToken.hpp"

namespace common {
namespace {
constexpr const char* TR = "TaskRunner";
}  // namespace

TaskRunner::TaskRunner() : mSlots() {
    uint16_t idx = 0U;
    for (auto& slot : mSlots) {
        slot.owner = this;
        slot.index = idx++;
    }
}

int TaskRunner::findFreeSlot() {
    for (size_t i = 0UL; i < mSlots.size(); ++i) {
        bool expected = false;
        if (mSlots[i].inUse.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

TaskHandle TaskRunner::start(const TaskParams& params, uint32_t stackWords, StepFn fn, void* user) {
    if (!fn || stackWords == 0U) {
        ESP_LOGE(TR, "start(%s): invalid args fn=%p stackWords=%u", params.name ? params.name : "?",
                 (void*)fn, (unsigned)stackWords);
        return {};
    }

    const int slotIdx = findFreeSlot();
    if (slotIdx < 0) {
        ESP_LOGE(TR, "start(%s): no free slot (MaxTasks=%u)", params.name ? params.name : "?",
                 (unsigned)MaxTasks);
        return {};
    }

    Slot& s = mSlots[static_cast<size_t>(slotIdx)];

    // Increment runId, wrapping to 1 on overflow so 0 remains reserved for invalid handles
    uint16_t next = static_cast<uint16_t>(s.runId.load(std::memory_order_relaxed) + 1U);
    if (next == 0U) {
        next = 1U;
    }

    s.runId.store(next, std::memory_order_release);
    s.stopRequested.store(false, std::memory_order_release);
    s.done.reset();

    s.fn = fn;
    s.user = user;

    std::strncpy(s.name, params.name ? params.name : "Task", sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';

    const BaseType_t res = xTaskCreatePinnedToCore(&TaskRunner::taskEntry, s.name, stackWords, &s,
                                                   static_cast<UBaseType_t>(params.priority),
                                                   &s.task, static_cast<BaseType_t>(params.core));

    if (res != pdPASS || s.task == nullptr) {
        ESP_LOGE(TR, "start(%s): xTaskCreatePinnedToCore FAILED", s.name);
        cleanupSlotFromTask(s);
        return {};
    }

    ESP_LOGI(TR, "start(%s): created task=%p slot=%d core=%d prio=%u stackWords=%u", s.name,
             (void*)s.task, slotIdx, (int)params.core, (unsigned)params.priority,
             (unsigned)stackWords);

    return TaskHandle{static_cast<uint16_t>(slotIdx), s.runId.load(std::memory_order_acquire)};
}

bool TaskRunner::validateHandle(const TaskHandle& h) const {
    if (!h.isValid() || h.slot >= MaxTasks) {
        return false;
    }

    const Slot& s = mSlots[h.slot];
    if (!s.inUse.load(std::memory_order_acquire)) {
        return false;
    }

    return (s.runId.load(std::memory_order_acquire) == h.runId);
}

bool TaskRunner::isStopRequested(const TaskHandle& h) const {
    if (!validateHandle(h)) {
        return true;
    }

    const Slot& s = mSlots[h.slot];
    return s.stopRequested.load(std::memory_order_acquire);
}

bool TaskRunner::interruptibleSleep(const TaskHandle& h, uint32_t ms) {
    if (isStopRequested(h)) {
        return true;
    }

    TickType_t ticks = common::toTicks(ms);
    if (ms > 0 && ticks == 0) {
        ticks = 1;  // Guarantee a yield
    }

    // Wake early if stop() calls xTaskNotifyGive()
    const uint32_t got = ulTaskNotifyTake(pdTRUE, ticks);
    return ((got > 0) || isStopRequested(h));
}

StopResult TaskRunner::stop(const TaskHandle& h, uint32_t waitMs) {
    if (!validateHandle(h)) {
        return StopResult::InvalidHandle;
    }

    Slot& s = mSlots[h.slot];
    s.stopRequested.store(true, std::memory_order_release);

    if (s.task) {
        (void)xTaskNotifyGive(s.task);
    }

    if (!s.done.wait(waitMs)) {
        return StopResult::Timeout;
    }

    return StopResult::Ok;
}

void TaskRunner::cleanupSlotFromTask(Slot& s) {
    s.task = nullptr;
    s.fn = nullptr;
    s.user = nullptr;

    s.stopRequested.store(false, std::memory_order_release);
    s.inUse.store(false, std::memory_order_release);

    s.done.signal();
}

void TaskRunner::taskEntry(void* arg) {
    auto* s = static_cast<Slot*>(arg);
    if (!s || !s->owner || !s->fn) {
        vTaskDelete(nullptr);
        return;
    }

    TaskRunner& runner = *s->owner;
    const TaskHandle h{s->index, s->runId.load(std::memory_order_acquire)};
    StopToken token(runner, h);
    TickType_t lastPrint = xTaskGetTickCount();

    ESP_LOGI(TR, "[%s] Task is running", s->name);

    while (!token.stopRequested()) {
        const StepResult r = s->fn(s->user, token);

        const TickType_t now = xTaskGetTickCount();
        if ((now - lastPrint) >= pdMS_TO_TICKS(30000)) {
            lastPrint = now;
            UBaseType_t hw = uxTaskGetStackHighWaterMark(nullptr);
            ESP_LOGI(TR, "[%s] stack high-water=%u words (%u bytes free)", s->name, (unsigned)hw,
                     (unsigned)(hw * sizeof(StackType_t)));
        }

        if (r.action == StepAction::Sleep) {
            if (token.sleepMs(r.sleepMs)) {
                break;
            }
            continue;
        }

        if (r.action == StepAction::Continue) {
            continue;
        }

        break;
    }

    ESP_LOGI(TR, "[%s] Task is exiting", s->name);

    runner.cleanupSlotFromTask(*s);
    vTaskDelete(nullptr);
}

}  // namespace common
