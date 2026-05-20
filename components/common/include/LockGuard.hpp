/**
 * @file LockGuard.hpp
 * @brief RAII wrapper for automatic mutex locking and unlocking.
 *
 * This file defines the LockGuard class, which ensures that a mutex is
 * released when the guard object's lifetime ends.
 */

#pragma once

#include "Mutex.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class LockGuard
 * @brief Scoped lock manager for the Mutex class.
 *
 * When a LockGuard is created, it attempts to acquire the provided mutex.
 * When it is destroyed (e.g., at the end of a function scope), it
 * automatically releases the lock if it was successfully acquired.
 */
class LockGuard {
   public:
    /**
     * @brief Constructs the guard and attempts to lock the mutex.
     * @param m Reference to the Mutex to be managed.
     * @param timeoutMs Maximum time to wait for the lock. Defaults to infinite (0xFFFFFFFF).
     */
    explicit LockGuard(Mutex& m, const uint32_t timeoutMs = 0xFFFFFFFF);

    /**
     * @brief Destructor that automatically unlocks the mutex if it is currently held.
     */
    ~LockGuard();

    /** @name Non-copyable
     * LockGuard cannot be copied to ensure unique ownership of the lock state.
     * @{ */
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
    /** @} */

   private:
    Mutex* mMutex; /**< Pointer to the managed mutex. */
    bool mLocked;  /**< Flag indicating if the lock is currently held by this guard. */
};

}  // namespace common
