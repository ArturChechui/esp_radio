#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "Types.hpp"

namespace adapters {
class IProvisioningPortal {
   public:
    virtual ~IProvisioningPortal() = default;

    virtual bool start(const common::ProvisioningPortalConfig& cfg,
                       common::CredentialsCallback onSubmit) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual std::string getApIp() const = 0;
};
}  // namespace adapters
