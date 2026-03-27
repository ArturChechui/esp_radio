#pragma once

#include <gmock/gmock.h>

#include "IWifiService.hpp"

namespace services {
class MockWifiService : public IWifiService {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, connect, (const uint32_t), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(std::string, getStatus, (), (const, override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, startProvisioningPortal, (), (override));
};
}  // namespace services
