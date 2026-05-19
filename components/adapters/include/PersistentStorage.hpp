/**
 * @file PersistentStorage.hpp
 * @brief Implementation of the IPersistentStorage interface using ESP32 NVS.
 *
 * This file contains the PersistentStorage class which manages key-value
 * pairs stored in the Flash memory's NVS partition.
 */

#pragma once

#include <cstdint>
#include <string>

#include "IPersistentStorage.hpp"
#include "Mutex.hpp"

// IDF
#include <nvs.h>

/**
 * @namespace adapters
 * @brief Contains hardware and system abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class PersistentStorage
 * @brief Concrete implementation of a persistent key-value store using NVS.
 *
 * This class wraps the ESP-IDF NVS library to provide a simple, thread-safe
 * interface for storing strings and integers. It handles namespace opening,
 * data committing, and error checking.
 */
class PersistentStorage final : public IPersistentStorage {
   public:
    /**
     * @brief Constructs a new PersistentStorage object.
     * Sets internal state to uninitialized.
     */
    PersistentStorage();

    /**
     * @brief Destroys the PersistentStorage object.
     * Ensures the NVS handle is closed if it was open.
     */
    ~PersistentStorage() override;

    /** @brief Deleted copy constructor to prevent unintended copying. */
    PersistentStorage(const PersistentStorage&) = delete;
    /** @brief Deleted assignment operator to prevent unintended copying. */
    PersistentStorage& operator=(const PersistentStorage&) = delete;

    /**
     * @brief Initializes the NVS partition and opens the default namespace.
     * @return true if NVS was successfully initialized and the handle is open, false otherwise.
     */
    bool init() override;

    /**
     * @brief Persists a string value in NVS.
     * @param key The key string (max 15 characters for NVS).
     * @param value The string content to save.
     * @return true if the write and commit were successful, false otherwise.
     */
    bool setString(const std::string& key, const std::string& value) override;

    /**
     * @brief Retrieves a string value from NVS.
     * @param key The key associated with the data.
     * @param value Reference to a string where the data will be stored.
     * @return true if the key was found and read successfully, false otherwise.
     */
    bool getString(const std::string& key, std::string& value) override;

    /**
     * @brief Persists an unsigned 32-bit integer in NVS.
     * @param key The key string.
     * @param value The integer value to save.
     * @return true if the write and commit were successful, false otherwise.
     */
    bool setU32(const std::string& key, uint32_t value) override;

    /**
     * @brief Retrieves an unsigned 32-bit integer from NVS.
     * @param key The key associated with the data.
     * @param value Reference to the variable where the data will be stored.
     * @return true if the key was found and read successfully, false otherwise.
     */
    bool getU32(const std::string& key, uint32_t& value) override;

    /**
     * @brief Removes a specific key and its data from NVS.
     * @param key The key to erase.
     * @return true if the key was erased and changes committed, false otherwise.
     */
    bool erase(const std::string& key) override;

   private:
    /**
     * @brief Internal check to ensure NVS is initialized and a handle is open.
     * @return true if the system is ready for data operations.
     */
    bool ensureReady() const;

    /**
     * @brief Commits (flushes) pending NVS changes to the physical Flash memory.
     * @return true if the commit operation succeeded.
     */
    bool commit();

    /** @brief The ESP-IDF NVS handle for the active namespace. */
    nvs_handle_t mHandle;

    /** @brief Internal flag tracking if the init() method was successful. */
    bool mReady;

    /** @brief Mutex to ensure thread-safe access to NVS operations. */
    common::Mutex mMutex;
};
}  // namespace adapters
