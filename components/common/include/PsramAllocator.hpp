/**
 * @file PsramAllocator.hpp
 * @brief Custom allocator for PSRAM memory management.
 *
 * This file defines the PsramAllocator class, which provides a custom allocator for managing memory
 * allocations in PSRAM. It ensures proper alignment and handles allocation failures gracefully.
 */

#pragma once

#include <cassert>
#include <cstdlib>

#if defined(ESP_PLATFORM)
// Real ESP-IDF build target
#include <esp_heap_caps.h>
#else
// Host unit tests (Linux / macOS / Windows)
#include <cstdlib>
#endif

#include <vector>

namespace {
constexpr size_t Alignment = 32U;  // Alignment for PSRAM allocations, ensuring proper
                                   // alignment for cache lines and performance.
}  // namespace

namespace common {

/**
 * @class PsramAllocator
 * @brief A custom allocator for managing allocations in PSRAM.
 */
template <typename T>
struct PsramAllocator {
    using value_type = T;

    /** @brief Default constructor. */
    PsramAllocator() = default;
    /** @brief Default destructor. */
    ~PsramAllocator() = default;

    /**
     * @brief Copy constructor for PsramAllocator. Required by std containers to convert
     * PsramAllocator<T> to PsramAllocator<U>.
     * @tparam U The type of the other allocator.
     * @param other The other PsramAllocator to copy from.
     */
    template <typename U>
    constexpr PsramAllocator(const PsramAllocator<U>&) noexcept {}

    /** @brief Allocates memory for the specified number of elements in PSRAM.
     * @param n The number of elements to allocate.
     * @return A pointer to the allocated memory.
     */
    T* allocate(std::size_t n) {
        void* ptr = nullptr;

#if defined(ESP_PLATFORM)
        // ESP32 Hardware Path: Allocate from PSRAM (SPIRAM) with 32-byte alignment
        ptr =
            heap_caps_aligned_alloc(Alignment, n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
        // Host Unit Test Path: Use standard aligned heap allocation
        ptr = ::aligned_alloc(Alignment, (n * sizeof(T) + Alignment - 1) & ~(Alignment - 1));
#endif

        if (!ptr) {
            assert(ptr != nullptr && "PsramAllocator failed to allocate memory!");
            std::abort();
        }

        return static_cast<T*>(ptr);
    }

    /**
     * @brief Deallocates memory in PSRAM.
     * @param p Pointer to the memory to deallocate.
     * @param n The number of elements to deallocate (ignored).
     */
    void deallocate(T* p, std::size_t) noexcept {
#if defined(ESP_PLATFORM)
        heap_caps_free(p);
#else
        ::free(p);
#endif
    }

    bool operator==(const PsramAllocator&) const noexcept {
        return true;
    }
    bool operator!=(const PsramAllocator&) const noexcept {
        return false;
    }
};
}  // namespace common
