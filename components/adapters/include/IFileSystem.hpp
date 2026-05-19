/**
 * @file IFileSystem.hpp
 * @brief Interface definition for filesystem operations.
 *
 * This file defines the abstract interface for managing files and partitions,
 * providing a consistent API for different storage backends.
 */

#pragma once

#include <cstddef>
#include <string>

/**
 * @namespace adapters
 * @brief Contains hardware and system abstraction layer implementation and interface classes.
 */
namespace adapters {

/**
 * @class IFileSystem
 * @brief Abstract interface for a filesystem controller.
 *
 * This interface defines the contract for implementing filesystem operations,
 * including partition mounting, file CRUD operations, and storage usage tracking.
 */
class IFileSystem {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IFileSystem() = default;

    /**
     * @brief Mounts and prepares the filesystem backend.
     * @return true if the filesystem was successfully initialized and mounted, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Unmounts the backend and releases all associated hardware resources.
     */
    virtual void deinit() = 0;

    /**
     * @brief Writes data to a file at a relative path under the mounted base path.
     * @param relativePath The path of the file relative to the mount point.
     * @param data The string content to be written to the file.
     * @return true if the write operation was successful, false otherwise.
     */
    virtual bool writeFile(const std::string& relativePath, const std::string& data) = 0;

    /**
     * @brief Reads the entire contents of a file into a string.
     * @param relativePath The path of the file relative to the mount point.
     * @param outData Reference to a string where the file content will be stored.
     * @return true if the file was read successfully, false otherwise.
     */
    virtual bool readFile(const std::string& relativePath, std::string& outData) = 0;

    /**
     * @brief Checks whether a file or directory exists at the specified relative path.
     * @param relativePath The path to check.
     * @return true if the path exists, false otherwise.
     */
    virtual bool exists(const std::string& relativePath) = 0;

    /**
     * @brief Deletes a file from the filesystem if it is present.
     * @param relativePath The path of the file to remove.
     * @return true if the file was successfully deleted, false otherwise.
     */
    virtual bool removeFile(const std::string& relativePath) = 0;

    /**
     * @brief Renames or moves a file within the mounted filesystem.
     * @param fromRelativePath The current relative path of the file.
     * @param toRelativePath The target relative path for the file.
     * @return true if the file was successfully renamed or moved, false otherwise.
     */
    virtual bool renameFile(const std::string& fromRelativePath,
                            const std::string& toRelativePath) = 0;

    /**
     * @brief Retrieves the total capacity and current used space for the mounted partition.
     * @param outTotalBytes Reference to store the total capacity in bytes.
     * @param outUsedBytes Reference to store the used space in bytes.
     * @return true if usage information was successfully retrieved, false otherwise.
     */
    virtual bool getUsage(size_t& outTotalBytes, size_t& outUsedBytes) = 0;

    /**
     * @brief Returns the mount point of the filesystem.
     * @return A string representing the base mount point (e.g., "/littlefs" or "/spiffs").
     */
    virtual std::string basePath() const = 0;
};
}  // namespace adapters
