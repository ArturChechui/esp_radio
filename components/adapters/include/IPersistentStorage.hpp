#pragma once

#include <cstdint>
#include <string>

namespace adapters {
class IPersistentStorage {
   public:
    virtual ~IPersistentStorage() = default;

    virtual bool init() = 0;
    virtual bool setString(const std::string& key, const std::string& value) = 0;
    virtual bool getString(const std::string& key, std::string& outVal) = 0;
    virtual bool setU32(const std::string& key, const uint32_t value) = 0;
    virtual bool getU32(const std::string& key, uint32_t& out) = 0;
    virtual bool erase(const std::string& key) = 0;
};
}  // namespace adapters
