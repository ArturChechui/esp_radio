#include "FileSystem.hpp"

#include <esp_littlefs.h>
#include <esp_log.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <utility>

#include "LockGuard.hpp"

namespace adapters {
namespace {
constexpr const char* Tag = "FileSystem";

constexpr const char* PartitionLabel = "storage";
constexpr const char* BasePath = "/littlefs";
}  // namespace

FileSystem::FileSystem()
    : mPartitionLabel(PartitionLabel), mBasePath(BasePath), mReady(false), mMutex() {}

FileSystem::~FileSystem() {
    deinit();
}

bool FileSystem::init() {
    common::LockGuard guard(mMutex);

    if (mReady) {
        return true;
    }

    esp_vfs_littlefs_conf_t cfg = {
        .base_path = mBasePath.c_str(),
        .partition_label = mPartitionLabel.empty() ? nullptr : mPartitionLabel.c_str(),
        .partition = nullptr,
        .format_if_mount_failed = true,
        .read_only = false,
        .dont_mount = false,
        .grow_on_mount = true};

    const esp_err_t err = esp_vfs_littlefs_register(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "esp_vfs_littlefs_register failed for partition '%s': %s",
                 mPartitionLabel.c_str(), esp_err_to_name(err));
        return false;
    }

    mReady = true;
    ESP_LOGI(Tag, "Mounted LittleFS at '%s' (partition='%s')", mBasePath.c_str(),
             mPartitionLabel.c_str());
    return true;
}

void FileSystem::deinit() {
    common::LockGuard guard(mMutex);

    if (!mReady) {
        return;
    }

    const esp_err_t err =
        esp_vfs_littlefs_unregister(mPartitionLabel.empty() ? nullptr : mPartitionLabel.c_str());
    if (err != ESP_OK) {
        ESP_LOGW(Tag, "esp_vfs_littlefs_unregister failed for partition '%s': %s",
                 mPartitionLabel.c_str(), esp_err_to_name(err));
    }

    mReady = false;
}

bool FileSystem::writeFile(const std::string& relativePath, const std::string& data) {
    common::LockGuard guard(mMutex);

    if (!ensureReady()) {
        return false;
    }

    const std::string path = makeAbsolutePath(relativePath);
    if (path.empty()) {
        return false;
    }

    FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        ESP_LOGE(Tag, "fopen write failed for '%s': errno=%d (%s)", path.c_str(), errno,
                 std::strerror(errno));
        return false;
    }

    const size_t written = std::fwrite(data.data(), 1U, data.size(), file);
    const int flushRc = std::fflush(file);
    const int closeRc = std::fclose(file);

    if (written != data.size() || flushRc != 0 || closeRc != 0) {
        ESP_LOGE(Tag, "write failed for '%s'", path.c_str());
        return false;
    }

    return true;
}

bool FileSystem::readFile(const std::string& relativePath, std::string& outData) {
    common::LockGuard guard(mMutex);

    if (!ensureReady()) {
        return false;
    }

    const std::string path = makeAbsolutePath(relativePath);
    if (path.empty()) {
        return false;
    }

    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        ESP_LOGE(Tag, "fopen read failed for '%s': errno=%d (%s)", path.c_str(), errno,
                 std::strerror(errno));
        return false;
    }

    if (std::fseek(file, 0L, SEEK_END) != 0) {
        std::fclose(file);
        return false;
    }

    const long size = std::ftell(file);
    if (size < 0L) {
        std::fclose(file);
        return false;
    }

    if (std::fseek(file, 0L, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }

    outData.resize(static_cast<size_t>(size));
    if (size > 0L) {
        const size_t readCount = std::fread(outData.data(), 1U, static_cast<size_t>(size), file);
        if (readCount != static_cast<size_t>(size)) {
            std::fclose(file);
            return false;
        }
    }

    if (std::fclose(file) != 0) {
        return false;
    }

    return true;
}

bool FileSystem::exists(const std::string& relativePath) {
    common::LockGuard guard(mMutex);

    if (!ensureReady()) {
        return false;
    }

    const std::string path = makeAbsolutePath(relativePath);
    if (path.empty()) {
        return false;
    }

    struct stat st{};
    return (::stat(path.c_str(), &st) == 0);
}

bool FileSystem::removeFile(const std::string& relativePath) {
    common::LockGuard guard(mMutex);

    if (!ensureReady()) {
        return false;
    }

    const std::string path = makeAbsolutePath(relativePath);
    if (path.empty()) {
        return false;
    }

    const int rc = ::unlink(path.c_str());
    if (rc == 0) {
        return true;
    }

    if (errno == ENOENT) {
        return true;
    }

    ESP_LOGE(Tag, "unlink failed for '%s': errno=%d (%s)", path.c_str(), errno,
             std::strerror(errno));
    return false;
}

bool FileSystem::renameFile(const std::string& fromRelativePath,
                            const std::string& toRelativePath) {
    common::LockGuard guard(mMutex);

    if (!ensureReady()) {
        return false;
    }

    const std::string fromPath = makeAbsolutePath(fromRelativePath);
    if (fromPath.empty()) {
        return false;
    }

    const std::string toPath = makeAbsolutePath(toRelativePath);
    if (toPath.empty()) {
        return false;
    }

    if (::rename(fromPath.c_str(), toPath.c_str()) == 0) {
        return true;
    }

    ESP_LOGE(Tag, "rename failed from '%s' to '%s': errno=%d (%s)", fromPath.c_str(),
             toPath.c_str(), errno, std::strerror(errno));

    return false;
}

bool FileSystem::getUsage(size_t& outTotalBytes, size_t& outUsedBytes) {
    common::LockGuard guard(mMutex);

    if (!ensureReady()) {
        return false;
    }

    size_t total = 0U;
    size_t used = 0U;
    const esp_err_t err = esp_littlefs_info(
        mPartitionLabel.empty() ? nullptr : mPartitionLabel.c_str(), &total, &used);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "esp_littlefs_info failed for partition '%s': %s", mPartitionLabel.c_str(),
                 esp_err_to_name(err));
        return false;
    }

    outTotalBytes = total;
    outUsedBytes = used;
    return true;
}

std::string FileSystem::basePath() const {
    return mBasePath;
}

bool FileSystem::ensureReady() const {
    if (!mReady) {
        ESP_LOGE(Tag, "FileSystem is not initialized");
        return false;
    }
    return true;
}

std::string FileSystem::makeAbsolutePath(const std::string& relativePath) const {
    if (relativePath.empty()) {
        ESP_LOGE(Tag, "Path is empty");
        return {};
    }

    size_t first = 0U;
    while (first < relativePath.size() && relativePath[first] == '/') {
        ++first;
    }

    if (first >= relativePath.size()) {
        ESP_LOGE(Tag, "Invalid path: '%s'", relativePath.c_str());
        return {};
    }

    std::string sanitized = relativePath.substr(first);
    return mBasePath + "/" + sanitized;
}

}  // namespace adapters
