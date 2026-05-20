/**
 * @file Mutex.hpp
 * @brief Portable wrapper for a mutual exclusion (mutex) primitive.
 *
 * This file defines the Mutex class, which provides basic synchronization
 * to prevent multiple threads from accessing shared data simultaneously.
 */

#pragma once

#include <cstdint>
#include <memory>

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class Mutex
 * @brief A standard mutual exclusion object.
 *
 * This class uses the Pimpl (Pointer to Implementation) idiom to hide
 * platform-specific threading details (like FreeRTOS semaphores or
 * POSIX mutexes) from the public interface.
 */
class Mutex {
   public:
    /**
     * @brief Constructs and initializes the underlying mutex primitive.
     */
    Mutex();

    /**
     * @brief Destroys the mutex and cleans up underlying resources.
     */
    ~Mutex();

    /** @name Non-copyable and Non-movable
     * Mutexes represent unique system resources and cannot be copied or moved.
     * @{ */
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;
    /** @} */

    /**
     * @brief Blocks the calling thread until ownership of the mutex is acquired.
     */
    void lock();

    /**
     * @brief Releases ownership of the mutex.
     */
    void unlock();

    /**
     * @brief Attempts to acquire the mutex without blocking indefinitely.
     * @param timeoutMs The maximum time to wait in milliseconds. Defaults to 0 (non-blocking).
     * @return true if the lock was successfully acquired, false otherwise.
     */
    bool tryLock(const uint32_t timeoutMs = 0);

    /**
     * @brief Checks if the underlying implementation was successfully initialized.
     * @return true if the mutex is valid and ready for use.
     */
    bool isValid() const;

   private:
    struct Impl;             /**< Forward declaration of the implementation structure. */
    std::unique_ptr<Impl> m; /**< Opaque pointer to the platform-specific implementation. */
};

}  // namespace common
