#pragma once

#include <cstdint>
#include <string>

namespace services {
class IWifiService {
   public:
    virtual ~IWifiService() = default;

    virtual bool init() = 0;
    virtual bool connect(const uint32_t timeoutMs = 30000) = 0;
    virtual bool isConnected() const = 0;
    virtual std::string getStatus() const = 0;
    virtual void disconnect() = 0;
    virtual bool startProvisioningPortal() = 0;
};
}  // namespace services
