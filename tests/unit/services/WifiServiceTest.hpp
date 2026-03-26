#pragma once

#include <gtest/gtest.h>

#include "MockEventQueue.hpp"
#include "MockPersistentStorage.hpp"
#include "MockProvisioningPortal.hpp"
#include "MockQueue.hpp"
#include "MockTaskRunner.hpp"
#include "MockWifiClient.hpp"
#include "Types.hpp"
#include "WifiService.hpp"

using ::testing::StrictMock;

class WifiServiceTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;
    void initSuccess();
    void connectSuccess();

    std::unique_ptr<StrictMock<adapters::MockWifiClient>> mockWifiClient;
    std::unique_ptr<StrictMock<adapters::MockProvisioningPortal>> mockProvisioningPortal;
    std::unique_ptr<StrictMock<adapters::MockPersistentStorage>> mockPersistentStorage;
    std::unique_ptr<StrictMock<common::MockEventQueue>> mockEventQueue;
    std::unique_ptr<StrictMock<common::MockTaskRunner>> mockTaskRunner;

    std::unique_ptr<services::WifiService> wifiService;

    common::WifiStateCallback wifiStateCb = nullptr;
    common::CredentialsCallback credsCb = nullptr;
    common::StepFn stepFn = nullptr;
    void *stepUser = nullptr;
};
