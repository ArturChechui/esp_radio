/**
 * @file IQueue.hpp
 * @brief Template interface for thread-safe queue operations.
 *
 * This file defines the IQueue interface, providing an abstraction for
 * message passing between different execution contexts.
 */

#pragma once

#include <cstdint>

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class IQueue
 * @brief Generic thread-safe queue interface.
 * * @tparam T The type of elements stored in the queue.
 */
template <typename T>
class IQueue {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IQueue() = default;

    /**
     * @brief Pushes an item into the queue.
     * @param item The data to be added to the queue.
     * @param timeoutTicks Maximum time to wait if the queue is full.
     * @return true if the item was successfully pushed.
     */
    virtual bool push(const T& item, const uint32_t timeoutTicks = 100) = 0;

    /**
     * @brief Retrieves and removes an item from the queue, blocking until data is available.
     * @param out Reference to store the retrieved item.
     * @return true if an item was successfully retrieved.
     */
    virtual bool get(T& out) = 0;

    /**
     * @brief Attempts to retrieve an item without blocking.
     * @param out Reference to store the retrieved item.
     * @return true if an item was available and retrieved.
     */
    virtual bool tryGet(T& out) = 0;
};
}  // namespace common
