/**
 * @file IPersistentStorage.hpp
 * @brief Interface definition for non-volatile key-value storage.
 *
 * This file defines the abstract interface for persistent storage, allowing
 * the application to save and retrieve settings across reboots.
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @namespace adapters
 * @brief Contains hardware and system abstraction layer implementation and interface classes.
 */
namespace adapters {

/**
 * @class IPersistentStorage
 * @brief Abstract interface for a persistent key-value store.
 *
 * This interface provides a simplified API for storing configuration data such as
 * Wi-Fi credentials, volume levels, or last-played station URLs in non-volatile memory.
 */
class IPersistentStorage {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IPersistentStorage() = default;

    /**
     * @brief Initializes the storage backend (e.g., opens NVS namespaces).
     * @return true if the storage is ready for use, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Stores a string value associated with a specific key.
     * @param key The unique identifier for the data.
     * @param value The string content to persist.
     * @return true if the value was successfully written, false otherwise.
     */
    virtual bool setString(const std::string& key, const std::string& value) = 0;

    /**
     * @brief Retrieves a string value associated with a specific key.
     * @param key The unique identifier for the data.
     * @param outVal Reference to a string where the retrieved value will be stored.
     * @return true if the key exists and the value was read, false otherwise.
     */
    virtual bool getString(const std::string& key, std::string& outVal) = 0;

    /**
     * @brief Stores an unsigned 32-bit integer associated with a specific key.
     * @param key The unique identifier for the data.
     * @param value The integer value to persist.
     * @return true if the value was successfully written, false otherwise.
     */
    virtual bool setU32(const std::string& key, const uint32_t value) = 0;

    /**
     * @brief Retrieves an unsigned 32-bit integer associated with a specific key.
     * @param key The unique identifier for the data.
     * @param out Reference to a variable where the retrieved integer will be stored.
     * @return true if the key exists and the value was read, false otherwise.
     */
    virtual bool getU32(const std::string& key, uint32_t& out) = 0;

    /**
     * @brief Erases the data associated with a specific key from storage.
     * @param key The unique identifier to remove.
     * @return true if the key was successfully erased or did not exist, false on hardware error.
     */
    virtual bool erase(const std::string& key) = 0;
};
}  // namespace adapters
