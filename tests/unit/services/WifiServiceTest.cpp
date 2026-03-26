#include "WifiServiceTest.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "BoardConfig.hpp"
#include "Events.hpp"
#include "Fonts.hpp"
#include "MockStopToken.hpp"
#include "Types.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {}  // namespace

void WifiServiceTest::SetUp() {
    mockEventQueue = std::make_unique<StrictMock<common::MockEventQueue>>();
    mockTaskRunner = std::make_unique<StrictMock<common::MockTaskRunner>>();

    mockWifiClient = std::make_unique<StrictMock<adapters::MockWifiClient>>();
    mockProvisioningPortal = std::make_unique<StrictMock<adapters::MockProvisioningPortal>>();
    mockPersistentStorage = std::make_unique<StrictMock<adapters::MockPersistentStorage>>();

    wifiService = std::make_unique<services::WifiService>(*mockWifiClient, *mockProvisioningPortal,
                                                          *mockTaskRunner, *mockEventQueue,
                                                          *mockPersistentStorage);
}

void WifiServiceTest::TearDown() {
    wifiService.reset();

    mockEventQueue.reset();
    mockTaskRunner.reset();
    mockWifiClient.reset();
    mockProvisioningPortal.reset();
    mockPersistentStorage.reset();
}

void WifiServiceTest::initSuccess() {
    EXPECT_CALL(*mockPersistentStorage, getString("wifi_ssid", _))
        .WillOnce([](const std::string& key, std::string& outVal) {
            outVal = "name";
            return true;
        });
    EXPECT_CALL(*mockPersistentStorage, getString("wifi_password", _))
        .WillOnce([](const std::string& key, std::string& outVal) {
            outVal = "pass";
            return true;
        });

    EXPECT_TRUE(wifiService->init());
}

void WifiServiceTest::connectSuccess() {
    EXPECT_CALL(*mockWifiClient, setStateCallback(_))
        .WillOnce([&](common::WifiStateCallback callback) { wifiStateCb = callback; });
    EXPECT_CALL(*mockWifiClient, connect(_)).WillOnce([&](const common::WifiCredentials& creds) {
        EXPECT_EQ(creds.ssid, "name");
        EXPECT_EQ(creds.password, "pass");
        return true;
    });
    EXPECT_CALL(*mockWifiClient, waitForConnection(30000)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    EXPECT_TRUE(wifiService->connect(30000));

    ASSERT_NE(stepFn, nullptr);
    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockWifiClient, tryGetRssiDbm()).WillOnce(Return(-50));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::WifiStateChangedEvent>(&event)) {
            EXPECT_EQ(e->bars, 3);
            EXPECT_TRUE(e->isConnected);
            return true;
        }

        return false;
    });
    stepFn(stepUser, token);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::WifiStateChangedEvent>(&event)) {
            EXPECT_EQ(e->bars, 3);
            EXPECT_TRUE(e->isConnected);
            return true;
        }

        return false;
    });
    common::WifiState data;
    data.isConnected = true;
    data.rssi = -30;
    wifiStateCb(data);

    EXPECT_CALL(*mockWifiClient, isConnected()).WillOnce(Return(true));
    EXPECT_TRUE(wifiService->isConnected());

    EXPECT_CALL(*mockWifiClient, getStatus()).WillOnce(Return("Connected"));
    EXPECT_EQ(wifiService->getStatus(), "Connected");
}

TEST_F(WifiServiceTest, tc01_init_success) {
    initSuccess();
}

TEST_F(WifiServiceTest, tc02_connect_signalStep_success) {
    initSuccess();
    connectSuccess();
}

TEST_F(WifiServiceTest, tc03_connect_disconnect_success) {
    initSuccess();
    connectSuccess();

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    EXPECT_CALL(*mockWifiClient, disconnect(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockProvisioningPortal, isRunning()).WillOnce(Return(false));
    wifiService->disconnect();
}

TEST_F(WifiServiceTest, tc04_connect_startProv_credCb_disconnect_connect_success) {
    initSuccess();
    EXPECT_CALL(*mockWifiClient, setStateCallback(_)).Times(1);
    EXPECT_CALL(*mockWifiClient, connect(_)).WillOnce(Return(false));
    EXPECT_FALSE(wifiService->connect(30000));

    EXPECT_CALL(*mockProvisioningPortal, start(_, _))
        .WillOnce([&](const common::ProvisioningPortalConfig& cfg, common::CredentialsCallback cb) {
            credsCb = cb;
            return true;
        });
    EXPECT_TRUE(wifiService->startProvisioningPortal());

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        bool res = false;
        if (const auto* e = std::get_if<common::WifiCredsReceivedEvent>(&event)) {
            res = true;
        }
        EXPECT_TRUE(res);
        return res;
    });

    EXPECT_CALL(*mockPersistentStorage, setString("wifi_ssid", "name")).WillOnce(Return(true));
    EXPECT_CALL(*mockPersistentStorage, setString("wifi_password", "pass")).WillOnce(Return(true));
    credsCb(common::WifiCredentials{"name", "pass"});

    EXPECT_CALL(*mockWifiClient, disconnect(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockProvisioningPortal, isRunning()).WillOnce(Return(true));
    EXPECT_CALL(*mockProvisioningPortal, stop()).Times(1);
    wifiService->disconnect();

    connectSuccess();
}

TEST_F(WifiServiceTest, tc05_connect_startProv_fail) {
    initSuccess();
    EXPECT_CALL(*mockWifiClient, setStateCallback(_)).Times(1);
    EXPECT_CALL(*mockWifiClient, connect(_)).WillOnce(Return(false));
    EXPECT_FALSE(wifiService->connect(30000));

    EXPECT_CALL(*mockProvisioningPortal, start(_, _)).WillOnce(Return(false));
    EXPECT_FALSE(wifiService->startProvisioningPortal());
}