/**
 * @file Queue.hpp
 * @brief Implementation of the IQueue interface using FreeRTOS queues.
 *
 * This file contains the template-based Queue class, which provides a
 * thread-safe wrapper around FreeRTOS queue handles for both task-to-task
 * and ISR-to-task communication.
 */

#pragma once

#include <esp_attr.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <type_traits>

#include "IQueue.hpp"

namespace common {

/**
 * @class Queue
 * @brief A thread-safe, template-based queue wrapping FreeRTOS queue handles.
 *
 * This class ensures that the type T is trivially copyable, which is a
 * requirement for safe usage with FreeRTOS's internal memcpy-based
 * queue operations, particularly when used within ISRs.
 * * @tparam T The type of elements stored in the queue.
 */
template <typename T>
class Queue : public IQueue<T> {
    static_assert(std::is_trivially_copyable<T>::value,
                  "Queue<T> requires trivially_copyable T for safe ISR use");

   public:
    /**
     * @brief Constructs a new Queue and initializes the underlying RTOS primitive.
     * @param name A string name for the queue (used in logs).
     * @param length The maximum number of elements the queue can hold.
     */
    Queue(const char* name, const size_t length = 50)
        : mName(name ? name : "Queue"), mLength(length), mQueue(nullptr) {
        mQueue = xQueueCreate(static_cast<UBaseType_t>(mLength), sizeof(T));
        if (!mQueue) {
            ESP_LOGE(Tag, "[%s] Failed to create queue (len=%u)", mName, (unsigned)mLength);
        } else {
            ESP_LOGD(Tag, "[%s] Queue created len=%u", mName, (unsigned)mLength);
        }
    }

    /** @name Non-copyable
     * Queues manage unique system handles and cannot be copied.
     * @{ */
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    /** @} */

    /**
     * @brief Destroys the queue, flushing all remaining items and deleting the RTOS handle.
     */
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

    /**
     * @brief Pushes an item into the queue. Blocks the calling task if full.
     * @param item The data to copy into the queue.
     * @param timeoutTicks Maximum time to wait for space in system ticks.
     * @return true if successfully pushed, false on timeout or error.
     */
    bool push(const T& item, const uint32_t timeoutTicks) override {
        if (!mQueue) {
            return false;
        }

        const BaseType_t res = xQueueSend(mQueue, &item, pdMS_TO_TICKS(timeoutTicks));
        return (res == pdPASS);
    }

    /**
     * @brief ISR-safe push: copies an item into the queue from an interrupt context.
     * * This method uses xQueueSendFromISR and automatically triggers a context
     * switch (portYIELD_FROM_ISR) if a higher-priority task is unblocked.
     * * @param item The data to copy into the queue.
     * @return true if successfully posted, false if the queue is full.
     */
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

    /**
     * @brief Retrieves an item from the queue, blocking indefinitely until data arrives.
     * @param out Reference to store the retrieved item.
     * @return true if an item was successfully received.
     */
    bool get(T& out) override {
        if (!mQueue) {
            return false;
        }

        const BaseType_t res = xQueueReceive(mQueue, &out, portMAX_DELAY);
        return (res == pdTRUE);
    }

    /**
     * @brief Attempts to retrieve an item without blocking.
     * @param out Reference to store the retrieved item.
     * @return true if an item was available and retrieved, false if the queue was empty.
     */
    bool tryGet(T& out) override {
        if (!mQueue) {
            return false;
        }

        const BaseType_t res = xQueueReceive(mQueue, &out, 0);
        return (res == pdTRUE);
    }

   private:
    static constexpr const char* Tag = "Queue"; /**< Logger tag for this class. */

    const char* mName;    /**< Descriptive name for debugging. */
    size_t mLength;       /**< Maximum capacity of the queue. */
    QueueHandle_t mQueue; /**< Underlying FreeRTOS queue handle. */
};

}  // namespace common
