#pragma once

#include <gmock/gmock.h>

#include "IProvisioningPortal.hpp"
#include "Types.hpp"

namespace adapters {
class MockProvisioningPortal : public IProvisioningPortal {
   public:
    MOCK_METHOD(bool, start, (const common::ProvisioningPortalConfig&, common::CredentialsCallback),
                (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(bool, isRunning, (), (const, override));
    MOCK_METHOD(std::string, getApIp, (), (const, override));
};

}  // namespace adapters
