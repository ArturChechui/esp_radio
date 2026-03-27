#pragma once

#include <cstddef>
#include <string>

namespace adapters {
class IFileSystem {
   public:
    virtual ~IFileSystem() = default;

    // Mounts and prepares filesystem backend.
    virtual bool init() = 0;

    // Unmounts backend and releases resources.
    virtual void deinit() = 0;

    // Writes file at relative path under mounted base path.
    virtual bool writeFile(const std::string& relativePath, const std::string& data) = 0;

    // Reads full file into string.
    virtual bool readFile(const std::string& relativePath, std::string& outData) = 0;

    // Checks whether file exists.
    virtual bool exists(const std::string& relativePath) = 0;

    // Deletes file if present.
    virtual bool removeFile(const std::string& relativePath) = 0;

    // Renames/moves file within the mounted filesystem.
    virtual bool renameFile(const std::string& fromRelativePath,
                            const std::string& toRelativePath) = 0;

    // Returns total and used bytes for mounted partition.
    virtual bool getUsage(size_t& outTotalBytes, size_t& outUsedBytes) = 0;

    // Returns mount point, e.g. "/littlefs".
    virtual std::string basePath() const = 0;
};
}  // namespace adapters
