#pragma once

#include <functional>
#include <optional>
#include <string>

#include "Types.hpp"

namespace adapters {
class IWifiClient {
   public:
    virtual ~IWifiClient() = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual bool connect(const common::WifiCredentials& creds) = 0;
    virtual bool waitForConnection(uint32_t timeoutMs = 30000) = 0;
    virtual bool disconnect(uint32_t timeoutMs = 3000) = 0;
    virtual bool startAccessPoint(const common::ProvisioningPortalConfig& cfg) = 0;
    virtual bool stopAccessPoint() = 0;
    virtual std::string getApIp() const = 0;
    virtual bool isConnected() const = 0;
    virtual std::string getStatus() const = 0;
    virtual void setStateCallback(common::WifiStateCallback callback) = 0;
    virtual std::optional<int8_t> tryGetRssiDbm() const = 0;
};

}  // namespace adapters
