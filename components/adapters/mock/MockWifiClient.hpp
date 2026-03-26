#pragma once

#include <gmock/gmock.h>

#include "IWifiClient.hpp"
#include "Types.hpp"

namespace adapters {
class MockWifiClient : public IWifiClient {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, connect, (const common::WifiCredentials& creds), (override));
    MOCK_METHOD(bool, waitForConnection, (uint32_t timeoutMs), (override));
    MOCK_METHOD(bool, disconnect, (uint32_t timeoutMs), (override));
    MOCK_METHOD(bool, startAccessPoint, (const common::ProvisioningPortalConfig& cfg), (override));
    MOCK_METHOD(bool, stopAccessPoint, (), (override));
    MOCK_METHOD(std::string, getApIp, (), (const, override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(std::string, getStatus, (), (const, override));
    MOCK_METHOD(void, setStateCallback, (common::WifiStateCallback callback), (override));
    MOCK_METHOD(void, deinit, (), (override));
    MOCK_METHOD(std::optional<int8_t>, tryGetRssiDbm, (), (const, override));
};

}  // namespace adapters
