#pragma once

#include <esp_attr.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <type_traits>

#include "IQueue.hpp"

namespace common {
template <typename T>
class Queue : public IQueue<T> {
    static_assert(std::is_trivially_copyable<T>::value,
                  "Queue<T> requires trivially_copyable T for safe ISR use");

   public:
    Queue(const char* name, const size_t length = 50)
        : mName(name ? name : "Queue"), mLength(length), mQueue(nullptr) {
        mQueue = xQueueCreate(static_cast<UBaseType_t>(mLength), sizeof(T));
        if (!mQueue) {
            ESP_LOGE(Tag, "[%s] Failed to create queue (len=%u)", mName, (unsigned)mLength);
        } else {
            ESP_LOGD(Tag, "[%s] Queue created len=%u", mName, (unsigned)mLength);
        }
    }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    ~Queue() override {
        if (mQueue) {
            T tmp;
            while (xQueueReceive(mQueue, &tmp, 0) == pdTRUE) {
                (void)tmp;
            }
            vQueueDelete(mQueue);
            mQueue = nullptr;
            ESP_LOGD(Tag, "[%s] Queue deleted", mName);
        }
    }

    bool push(const T& item, const uint32_t timeoutTicks) override {
        if (!mQueue) {
            return false;
        }

        const BaseType_t res = xQueueSend(mQueue, &item, pdMS_TO_TICKS(timeoutTicks));
        return (res == pdPASS);
    }

    // ISR-safe push: copies a trivially-copyable `item` into the RTOS queue via xQueueSendFromISR
    // and returns true on success.
    // If `higherPriorityWoken` is nullptr the function will call portYIELD_FROM_ISR() when
    // required; otherwise the caller must call portYIELD_FROM_ISR() if *higherPriorityWoken ==
    // pdTRUE.
    bool IRAM_ATTR pushFromIsr(const T& item) {
        if (!mQueue) {
            return false;
        }

        BaseType_t hpw = pdFALSE;
        const BaseType_t res = xQueueSendFromISR(mQueue, &item, &hpw);
        if (hpw) {
            portYIELD_FROM_ISR();
        }

        return (res == pdTRUE);
    }

    // TODO: Combine methods??
    bool get(T& out) override {
        if (!mQueue) {
            return false;
        }

        const BaseType_t res = xQueueReceive(mQueue, &out, portMAX_DELAY);
        return (res == pdTRUE);
    }

    bool tryGet(T& out) override {
        if (!mQueue) {
            return false;
        }

        const BaseType_t res = xQueueReceive(mQueue, &out, 0);
        return (res == pdTRUE);
    }

   private:
    static constexpr const char* Tag = "Queue";

    const char* mName;
    size_t mLength;
    QueueHandle_t mQueue;
};

}  // namespace common
