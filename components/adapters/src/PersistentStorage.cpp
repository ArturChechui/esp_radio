#include "PersistentStorage.hpp"

#include <utility>

#include "LockGuard.hpp"

// IDF
#include <esp_log.h>

namespace adapters {
namespace {
constexpr const char* Tag = "PersistentStorage";
constexpr const char* Namespace = "esp_radio";
}  // namespace

PersistentStorage::PersistentStorage() : mHandle(0), mReady(false), mMutex() {
    ESP_LOGI(Tag, "Creating PersistentStorage");
}

PersistentStorage::~PersistentStorage() {
    if (mReady) {
        nvs_close(mHandle);
        mReady = false;
    }
}

bool PersistentStorage::init() {
    common::LockGuard guard(mMutex);

    if (mReady) {
        return true;
    }

    esp_err_t err = nvs_open(Namespace, NVS_READWRITE, &mHandle);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_open failed for namespace='%s': %s", Namespace, esp_err_to_name(err));
        return false;
    }

    mReady = true;
    ESP_LOGI(Tag, "PersistentStorage initialized");

    return true;
}

bool PersistentStorage::setString(const std::string& key, const std::string& value) {
    common::LockGuard guard(mMutex);

    if (!ensureReady() || key.empty()) {
        return false;
    }

    const esp_err_t err = nvs_set_str(mHandle, key.c_str(), value.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_set_str failed for key='%s': %s", key.c_str(), esp_err_to_name(err));
        return false;
    }

    return commit();
}

bool PersistentStorage::getString(const std::string& key, std::string& outVal) {
    common::LockGuard guard(mMutex);

    if (!ensureReady() || key.empty()) {
        return false;
    }

    // First call returns required buffer size (includes trailing '\0').
    size_t requiredSize = 0;
    esp_err_t err = nvs_get_str(mHandle, key.c_str(), nullptr, &requiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return false;
    } else if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_get_str(size) failed for key='%s': %s", key.c_str(),
                 esp_err_to_name(err));
        return false;
    }

    std::string out(requiredSize, '\0');
    err = nvs_get_str(mHandle, key.c_str(), out.data(), &requiredSize);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_get_str(value) failed for key='%s': %s", key.c_str(),
                 esp_err_to_name(err));
        return false;
    }

    if (!out.empty() && out.back() == '\0') {
        out.pop_back();
    }
    outVal = std::move(out);
    return true;
}

bool PersistentStorage::setU32(const std::string& key, const uint32_t value) {
    common::LockGuard guard(mMutex);

    if (!ensureReady() || key.empty()) {
        return false;
    }

    const esp_err_t err = nvs_set_u32(mHandle, key.c_str(), value);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_set_u32 failed for key='%s': %s", key.c_str(), esp_err_to_name(err));
        return false;
    }

    return commit();
}

bool PersistentStorage::getU32(const std::string& key, uint32_t& out) {
    common::LockGuard guard(mMutex);

    if (!ensureReady() || key.empty()) {
        return false;
    }

    const esp_err_t err = nvs_get_u32(mHandle, key.c_str(), &out);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return false;
    } else if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_get_u32 failed for key='%s': %s", key.c_str(), esp_err_to_name(err));
        return false;
    }

    return true;
}

bool PersistentStorage::erase(const std::string& key) {
    common::LockGuard guard(mMutex);

    if (!ensureReady() || key.empty()) {
        return false;
    }

    const esp_err_t err = nvs_erase_key(mHandle, key.c_str());
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    } else if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_erase_key failed for key='%s': %s", key.c_str(), esp_err_to_name(err));
        return false;
    }

    return commit();
}

bool PersistentStorage::ensureReady() const {
    if (!mReady) {
        ESP_LOGE(Tag, "PersistentStorage not initialized");
        return false;
    }
    return true;
}

bool PersistentStorage::commit() {
    const esp_err_t err = nvs_commit(mHandle);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "nvs_commit failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

}  // namespace adapters
