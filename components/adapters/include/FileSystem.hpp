/**
 * @file FileSystem.hpp
 * @brief Implementation of the IFileSystem interface for managing file operations.
 * * This file contains the FileSystem class which provides a concrete implementation
 * for file handling, including reading, writing, and partition management.
 */

#pragma once

#include <cstddef>
#include <string>

#include "IFileSystem.hpp"
#include "Mutex.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and system abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class FileSystem
 * @brief Concrete implementation of a file system controller.
 * * This class handles low-level file operations such as initialization of the storage
 * partition, file CRUD (Create, Read, Update, Delete) operations, and usage statistics.
 * It ensures thread-safety using an internal mutex.
 */
class FileSystem final : public IFileSystem {
   public:
    /**
     * @brief Constructs a new FileSystem object.
     */
    explicit FileSystem();

    /**
     * @brief Destroys the FileSystem object and ensures proper deinitialization.
     */
    ~FileSystem() override;

    /** @brief Deleted copy constructor to prevent unintended copying. */
    FileSystem(const FileSystem&) = delete;
    /** @brief Deleted assignment operator to prevent unintended copying. */
    FileSystem& operator=(const FileSystem&) = delete;

    /**
     * @brief Initializes the file system and mounts the storage partition.
     * @return true if the file system was successfully mounted and is ready, false otherwise.
     */
    bool init() override;

    /**
     * @brief Deinitializes the file system and unmounts the partition.
     */
    void deinit() override;

    /**
     * @brief Writes data to a file at the specified relative path.
     * @param relativePath The path of the file relative to the base mount point.
     * @param data The string content to write to the file.
     * @return true if the write operation was successful, false otherwise.
     */
    bool writeFile(const std::string& relativePath, const std::string& data) override;

    /**
     * @brief Reads the contents of a file into a string.
     * @param relativePath The path of the file relative to the base mount point.
     * @param outData Reference to a string where the file content will be stored.
     * @return true if the file was read successfully, false otherwise.
     */
    bool readFile(const std::string& relativePath, std::string& outData) override;

    /**
     * @brief Checks if a file or directory exists at the given path.
     * @param relativePath The path to check.
     * @return true if it exists, false otherwise.
     */
    bool exists(const std::string& relativePath) override;

    /**
     * @brief Removes a file from the file system.
     * @param relativePath The path of the file to remove.
     * @return true if the file was successfully deleted, false otherwise.
     */
    bool removeFile(const std::string& relativePath) override;

    /**
     * @brief Renames or moves a file.
     * @param fromRelativePath The current relative path of the file.
     * @param toRelativePath The new relative path for the file.
     * @return true if the file was successfully renamed, false otherwise.
     */
    bool renameFile(const std::string& fromRelativePath,
                    const std::string& toRelativePath) override;

    /**
     * @brief Retrieves the total and used storage space of the partition.
     * @param outTotalBytes Reference to store the total capacity in bytes.
     * @param outUsedBytes Reference to store the used space in bytes.
     * @return true if usage information was successfully retrieved, false otherwise.
     */
    bool getUsage(size_t& outTotalBytes, size_t& outUsedBytes) override;

    /**
     * @brief Returns the base mount point path of the file system.
     * @return A string representing the base path (e.g., "/spiffs").
     */
    std::string basePath() const override;

   private:
    /**
     * @brief Internal check to ensure the file system is initialized and mounted.
     * @return true if the system is ready for operations.
     */
    bool ensureReady() const;

    /**
     * @brief Converts a relative path into a full absolute system path.
     * @param relativePath The user-provided relative path.
     * @return The complete absolute path including the base mount point.
     */
    std::string makeAbsolutePath(const std::string& relativePath) const;

    /** @brief The label of the storage partition being managed. */
    std::string mPartitionLabel;

    /** @brief The root directory where the file system is mounted. */
    std::string mBasePath;

    /** @brief Internal flag tracking the initialization state. */
    bool mReady;

    /** @brief Mutex to ensure thread-safe access to file operations. */
    common::Mutex mMutex;
};

}  // namespace adapters
