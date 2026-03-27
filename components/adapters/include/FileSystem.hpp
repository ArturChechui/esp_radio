#pragma once

#include <cstddef>
#include <string>

#include "IFileSystem.hpp"
#include "Mutex.hpp"

namespace adapters {
class FileSystem final : public IFileSystem {
   public:
    explicit FileSystem();
    ~FileSystem() override;

    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    bool init() override;
    void deinit() override;
    bool writeFile(const std::string& relativePath, const std::string& data) override;
    bool readFile(const std::string& relativePath, std::string& outData) override;
    bool exists(const std::string& relativePath) override;
    bool removeFile(const std::string& relativePath) override;
    bool renameFile(const std::string& fromRelativePath, const std::string& toRelativePath) override;
    bool getUsage(size_t& outTotalBytes, size_t& outUsedBytes) override;
    std::string basePath() const override;

   private:
    bool ensureReady() const;
    std::string makeAbsolutePath(const std::string& relativePath) const;

    std::string mPartitionLabel;
    std::string mBasePath;
    bool mReady;
    common::Mutex mMutex;
};
}  // namespace adapters
