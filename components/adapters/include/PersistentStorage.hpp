#pragma once

#include <cstdint>
#include <string>

#include "IPersistentStorage.hpp"
#include "Mutex.hpp"

// IDF
#include <nvs.h>

namespace adapters {
class PersistentStorage final : public IPersistentStorage {
   public:
    PersistentStorage();
    ~PersistentStorage() override;

    PersistentStorage(const PersistentStorage&) = delete;
    PersistentStorage& operator=(const PersistentStorage&) = delete;

    bool init() override;
    bool setString(const std::string& key, const std::string& value) override;
    bool getString(const std::string& key, std::string& value) override;
    bool setU32(const std::string& key, uint32_t value) override;
    bool getU32(const std::string& key, uint32_t& value) override;
    bool erase(const std::string& key) override;

   private:
    bool ensureReady() const;
    bool commit();

    nvs_handle_t mHandle;
    bool mReady;
    common::Mutex mMutex;
};
}  // namespace adapters
