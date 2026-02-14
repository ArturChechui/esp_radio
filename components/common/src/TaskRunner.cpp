#include "TaskRunner.hpp"

#include <esp_heap_caps.h>
#include <esp_log.h>

#include <cstdlib>

#include "Helper.hpp"
#include "StopToken.hpp"

namespace common {
namespace {
static void* allocTcbMem(size_t bytes) {
    return heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

static void* allocStackMem(size_t bytes) {
    // Prefer INTERNAL for small stacks, fallback to PSRAM for bigger ones
    void* p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!p) {
        p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    return p;
}

static void freeMem(void* p) {
    heap_caps_free(p);
}

constexpr const char* TR = "TaskRunner";
// Remove all the logs or add to a debug mode
// static void logHeap(const char* where) {
//     ESP_LOGI("TaskRunner", "[%s] int: free=%u largest=%u | psram: free=%u largest=%u", where,
//              (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
//              (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
//              (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
//              (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
// }
}  // namespace

TaskRunner::TaskRunner() : mSlots() {
    for (size_t i = 0UL; i < MaxTasks; ++i) {
        mSlots[i].owner = this;
        mSlots[i].index = static_cast<uint16_t>(i);
    }
}

int TaskRunner::findFreeSlot() {
    for (size_t i = 0UL; i < MaxTasks; ++i) {
        bool expected = false;
        if (mSlots[i].inUse.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

TaskHandle TaskRunner::start(const TaskParams& params, uint32_t stackWords, StepFn fn, void* user) {
    if (!fn || stackWords == 0U) {
        const size_t stackBytes = stackWords * sizeof(StackType_t);
        ESP_LOGE(TR, "start(%s): invalid args fn=%p stackWords=%u (%u bytes)",
                 params.name ? params.name : "?", (void*)fn, (unsigned)stackWords,
                 (unsigned)stackBytes);
        return {};
    }

    // logHeap("before slot");

    const int slotIdx = findFreeSlot();
    if (slotIdx < 0) {
        ESP_LOGE(TR, "start(%s): no free slot (MaxTasks=%u)", params.name ? params.name : "?",
                 (unsigned)MaxTasks);
        // optionally dump slot states here
        return {};
    }

    Slot& s = mSlots[static_cast<size_t>(slotIdx)];
    ESP_LOGI(TR, "start(%s): slot=%d core=%d prio=%u stackWords=%u (%u bytes)",
             params.name ? params.name : "?", slotIdx, (int)params.core, (unsigned)params.priority,
             (unsigned)stackWords, (unsigned)(stackWords * sizeof(StackType_t)));

    // bump runId (never 0)
    uint16_t next = static_cast<uint16_t>(s.runId.load(std::memory_order_relaxed) + 1U);
    if (next == 0U) {
        next = 1U;
    }

    s.runId.store(next, std::memory_order_release);

    s.stopRequested.store(false, std::memory_order_release);
    s.done.reset();

    s.fn = fn;
    s.user = user;
    s.stackWords = stackWords;

    std::strncpy(s.name, params.name ? params.name : "Task", sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';

    // TODO: come up with a better way of realocattion of the memory, maybe reuse?
    // or just change to xTaskCreatePinnedToCore ?
    if (s.tcb) {
        freeMem(s.tcb);
        s.tcb = nullptr;
    }
    if (s.stack) {
        freeMem(s.stack);
        s.stack = nullptr;
    }

    // logHeap("before alloc");

    // allocate new internal task memory
    s.tcb = static_cast<StaticTask_t*>(allocTcbMem(sizeof(StaticTask_t)));
    s.stack = static_cast<StackType_t*>(allocStackMem(sizeof(StackType_t) * stackWords));

    if (!s.tcb || !s.stack) {
        ESP_LOGE(TR, "start(%s): alloc failed tcb=%p stack=%p", s.name, (void*)s.tcb,
                 (void*)s.stack);
        // logHeap("alloc failed");

        freeMem(s.tcb);
        freeMem(s.stack);
        s.tcb = nullptr;
        s.stack = nullptr;
        s.fn = nullptr;
        s.user = nullptr;
        s.inUse.store(false, std::memory_order_release);
        return {};
    }

    // logHeap("before create");

    // Create task
    s.task = xTaskCreateStaticPinnedToCore(&TaskRunner::taskEntry, s.name, s.stackWords, &s,
                                           static_cast<UBaseType_t>(params.priority), s.stack,
                                           s.tcb, static_cast<BaseType_t>(params.core));

    if (s.task == nullptr) {
        ESP_LOGE(TR, "start(%s): xTaskCreateStaticPinnedToCore FAILED", s.name);
        // logHeap("create failed");

        freeMem(s.tcb);
        freeMem(s.stack);
        s.tcb = nullptr;
        s.stack = nullptr;
        s.fn = nullptr;
        s.user = nullptr;
        s.inUse.store(false, std::memory_order_release);
        return {};
    }

    ESP_LOGI(TR, "start(%s): created task=%p", s.name, (void*)s.task);

    return TaskHandle{static_cast<uint16_t>(slotIdx), s.runId.load(std::memory_order_acquire)};
}

bool TaskRunner::validateHandle(const TaskHandle& h) const {
    if (!h.isValid()) {
        return false;
    } else if (h.slot >= MaxTasks) {
        return false;
    }

    const Slot& s = mSlots[h.slot];
    if (!s.inUse.load(std::memory_order_acquire)) {
        return false;
    }

    return (s.runId.load(std::memory_order_acquire) == h.runId);
}

bool TaskRunner::isStopRequested(const TaskHandle& h) const {
    if (!h.isValid() || h.slot >= MaxTasks) {
        return true;
    }

    const Slot& s = mSlots[h.slot];
    if (!s.inUse.load(std::memory_order_acquire)) {
        return true;
    } else if (s.runId.load(std::memory_order_acquire) != h.runId) {
        return true;
    }

    return s.stopRequested.load(std::memory_order_acquire);
}

bool TaskRunner::interruptibleSleep(const TaskHandle& h, uint32_t ms) {
    if (isStopRequested(h)) {
        return true;
    }

    TickType_t ticks = common::toTicks(ms);
    if (ms > 0 && ticks == 0) {
        ticks = 1;  // guarantee a real block/yield
    }

    // stop() uses xTaskNotifyGive() to wake us early
    const uint32_t got = ulTaskNotifyTake(pdTRUE, ticks);
    return (got > 0) || isStopRequested(h);
}

StopResult TaskRunner::stop(const TaskHandle& h, uint32_t waitMs) {
    if (!validateHandle(h)) {
        return StopResult::InvalidHandle;
    }

    Slot& s = mSlots[h.slot];
    s.stopRequested.store(true, std::memory_order_release);

    // Wake task if it's in sleepMs()
    if (s.task) {
        (void)xTaskNotifyGive(s.task);
    }

    if (!s.done.wait(waitMs)) {
        return StopResult::Timeout;
    }

    return StopResult::Ok;
}

void TaskRunner::cleanupSlotFromTask(Slot& s) {
    // TODO: Correctly free stack or change to simple task creation
    // Free internal allocations owned by runner
    // freeMem(s.tcb);
    // freeMem(s.stack);
    // s.tcb = nullptr;
    // s.stack = nullptr;
    // s.stackWords = 0;

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
    TickType_t lastPrint = 0;
    ESP_LOGI(TR, "[%s] Task is running", s->name);

    while (!token.stopRequested()) {
        const StepResult r = s->fn(s->user, token);

        const TickType_t now = xTaskGetTickCount();
        if ((now - lastPrint) >= pdMS_TO_TICKS(30000)) {
            lastPrint = now;
            UBaseType_t hw = uxTaskGetStackHighWaterMark(nullptr);
            ESP_LOGI(TR, "[%s] high-water=%u words (%u bytes free)", s->name, (unsigned)hw,
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
