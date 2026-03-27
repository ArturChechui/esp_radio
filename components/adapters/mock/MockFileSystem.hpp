#pragma once

#include <gmock/gmock.h>

#include "IFileSystem.hpp"

namespace adapters {
class MockFileSystem : public IFileSystem {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(void, deinit, (), (override));
    MOCK_METHOD(bool, writeFile, (const std::string& relativePath, const std::string& data),
                (override));
    MOCK_METHOD(bool, readFile, (const std::string& relativePath, std::string& outData),
                (override));
    MOCK_METHOD(bool, exists, (const std::string& relativePath), (override));
    MOCK_METHOD(bool, removeFile, (const std::string& relativePath), (override));
    MOCK_METHOD(bool, renameFile,
                (const std::string& fromRelativePath, const std::string& toRelativePath),
                (override));
    MOCK_METHOD(bool, getUsage, (size_t& outTotalBytes, size_t& outUsedBytes), (override));
    MOCK_METHOD(std::string, basePath, (), (const, override));
};
}  // namespace adapters
