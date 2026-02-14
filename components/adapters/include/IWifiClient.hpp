#pragma once

#include <functional>
#include <optional>
#include <string>

#include "Types.hpp"

namespace adapters {
class IWifiClient {
   public:
    virtual ~IWifiClient() = default;

    virtual bool init(const std::string& ssid, const std::string& password) = 0;
    virtual bool waitForConnection(uint32_t timeoutMs = 30000) = 0;
    virtual bool isConnected() const = 0;
    virtual std::string getStatus() const = 0;
    virtual void setStateCallback(common::WifiStateCallback callback) = 0;
    virtual void deinit() = 0;

    virtual std::optional<int8_t> tryGetRssiDbm() const = 0;
};

}  // namespace adapters
