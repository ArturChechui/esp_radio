#include "AppControllerTest.hpp"

#include "Events.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {
constexpr const char* ManifestJson = R"({"version":"1.0.1"})";
constexpr const char* LocalManifestJson = R"({"version":"1.0.0"})";
constexpr const char* StationsJson =
    R"([{"id":"id1","name":"station1","url":"https://example.com"}])";
constexpr const char* ManifestUrl =
    "https://raw.githubusercontent.com/ArturChechui/esp_radio/refs/heads/feature/FR-07/resources/"
    "json/manifest.json";
constexpr const char* StationsUrl =
    "https://raw.githubusercontent.com/ArturChechui/esp_radio/refs/heads/feature/FR-07/resources/"
    "json/stations.json";

constexpr const char* ManifestPath = "manifest.json";
constexpr const char* ManifestTmpPath = "manifest.new.json";
constexpr const char* ManifestBackupPath = "manifest.bak.json";

constexpr const char* StationsPath = "stations.json";
constexpr const char* StationsTmpPath = "stations.new.json";
constexpr const char* StationsBackupPath = "stations.bak.json";

}  // namespace

void AppControllerTest::SetUp() {
    mockWifiService = std::make_unique<services::MockWifiService>();
    mockEventQueue = std::make_unique<common::MockEventQueue>();
    mockPlayerService = std::make_unique<services::MockPlayerService>();
    mockStationRepo = std::make_unique<services::MockStationRepository>();
    mockSensorService = std::make_unique<services::MockSensorService>();
    mockInputService = std::make_unique<services::MockInputService>();
    mockHttpClient = std::make_unique<adapters::MockHttpClient>();
    mockFileSystem = std::make_unique<adapters::MockFileSystem>();
    mockJsonParser = std::make_unique<common::MockJsonParser>();

    appController = std::make_unique<core::AppController>(
        *mockWifiService, *mockPlayerService, *mockStationRepo, *mockSensorService,
        *mockInputService, *mockHttpClient, *mockFileSystem, *mockJsonParser, *mockEventQueue);
}

void AppControllerTest::TearDown() {
    appController.reset();

    mockPlayerService.reset();
    mockStationRepo.reset();
    mockSensorService.reset();
    mockInputService.reset();
    mockHttpClient.reset();
    mockFileSystem.reset();
    mockJsonParser.reset();
    mockEventQueue.reset();
    mockWifiService.reset();
}

// TODO: improve repetition - make funcs etc
TEST_F(AppControllerTest, tc01_sysInitedEvent_wifiConnect_switchToMain) {
    EXPECT_CALL(*mockWifiService, connect(_)).WillOnce(Return(true));
    common::SystemInitedEvent e{};
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            bool res = false;
            if (const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event)) {
                res = true;
            }
            EXPECT_TRUE(res);
            return res;
        })
        .WillOnce([](const common::AppEvent& event) {
            bool res = false;
            if (const auto* e = std::get_if<common::WifiStateChangedEvent>(&event)) {
                EXPECT_EQ(e->bars, 1);
                EXPECT_TRUE(e->isConnected);
                res = true;
            }
            EXPECT_TRUE(res);
            return res;
        });

    common::WifiStateChangedEvent e2{};
    e2.bars = 1;
    e2.isConnected = true;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc02_sysInitedEvent_wifiConnect_switchToProv_switchToMain) {
    EXPECT_CALL(*mockWifiService, connect(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockWifiService, startProvisioningPortal()).WillOnce(Return(true));
    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            bool res = false;
            if (const auto* e = std::get_if<common::SwitchToWifiProvScreenEvent>(&event)) {
                res = true;
            }
            EXPECT_TRUE(res);
            return res;
        })
        .WillOnce([](const common::AppEvent& event) {
            bool res = false;
            if (const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event)) {
                res = true;
            }
            EXPECT_TRUE(res);
            return res;
        })
        .WillOnce([](const common::AppEvent& event) {
            bool res = false;
            if (const auto* e = std::get_if<common::WifiStateChangedEvent>(&event)) {
                EXPECT_EQ(e->bars, 1);
                EXPECT_TRUE(e->isConnected);
                res = true;
            }
            EXPECT_TRUE(res);
            return res;
        });

    common::SystemInitedEvent e{};
    appController->onEvent(e);

    EXPECT_CALL(*mockWifiService, disconnect()).Times(1);
    EXPECT_CALL(*mockWifiService, connect(_)).WillOnce(Return(true));
    common::WifiCredsReceivedEvent e2{};
    appController->onEvent(e2);

    common::WifiStateChangedEvent e3{};
    e3.bars = 1;
    e3.isConnected = true;
    appController->onEvent(e3);
}

TEST_F(AppControllerTest, tc03_sysInitedEvent_wifiConnect_switchToProv_wifiConnect_fail) {
    EXPECT_CALL(*mockWifiService, connect(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockWifiService, startProvisioningPortal()).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        bool res = false;
        if (const auto* e = std::get_if<common::SwitchToWifiProvScreenEvent>(&event)) {
            res = true;
        }
        EXPECT_TRUE(res);
        return res;
    });

    common::SystemInitedEvent e{};
    appController->onEvent(e);

    EXPECT_CALL(*mockWifiService, disconnect()).Times(1);
    EXPECT_CALL(*mockWifiService, connect(_)).WillOnce(Return(false));
    common::WifiCredsReceivedEvent e2{};
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc04_sysInitedEvent_startProvisioningPortal_fail) {
    EXPECT_CALL(*mockWifiService, connect(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockWifiService, startProvisioningPortal()).WillOnce(Return(false));
    common::SystemInitedEvent e{};
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc05_tempEvent) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::TempHumidUpdateEvent e{};
    e.humidity = 45;
    e.temperature = 11.0;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc06_wifiEvent) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::WifiStateChangedEvent e{};
    e.bars = 3;
    e.isConnected = true;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc07_playbackStatusEvent) {
    EXPECT_CALL(*mockSensorService, setPlaybackActive(true)).Times(1);
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e{};
    e.status = common::PlaybackStatus::Playing;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc08_buttonEvent_playStop_playStation_buffering) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));
    const common::StationData station = {.id = "id", .name = "name", .url = "url"};
    EXPECT_CALL(*mockStationRepo, currentStation()).WillOnce(ReturnRef(station));
    EXPECT_CALL(*mockPlayerService, playStation(station.url)).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e2{};
    e2.status = common::PlaybackStatus::Buffering;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc09_buttonEvent_playStop_playStation_playing) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));
    const common::StationData station = {.id = "id", .name = "name", .url = "url"};
    EXPECT_CALL(*mockStationRepo, currentStation()).WillOnce(ReturnRef(station));
    EXPECT_CALL(*mockPlayerService, playStation(station.url)).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e2{};
    e2.status = common::PlaybackStatus::Playing;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc10_buttonEvent_playStop_stop) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Playing));
    EXPECT_CALL(*mockPlayerService, stop()).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e2{};
    e2.status = common::PlaybackStatus::Stopped;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc11_buttonEvent_next_stop_nextStation_playStation) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Playing));
    EXPECT_CALL(*mockPlayerService, stop()).WillOnce(Return(true));
    const common::StationData station = {.id = "id", .name = "name", .url = "url"};
    EXPECT_CALL(*mockStationRepo, nextStation()).WillOnce(ReturnRef(station));
    EXPECT_CALL(*mockPlayerService, playStation(station.url)).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::Next;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::WifiStateChangedEvent e2{};
    e2.isConnected = false;
    e2.bars = 0;
    appController->onEvent(e2);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e3{};
    e3.status = common::PlaybackStatus::Playing;
    appController->onEvent(e3);
}

TEST_F(AppControllerTest, tc12_buttonEvent_next_nextStation_playStation) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));
    const common::StationData station = {.id = "id", .name = "name", .url = "url"};
    EXPECT_CALL(*mockStationRepo, nextStation()).WillOnce(ReturnRef(station));
    EXPECT_CALL(*mockPlayerService, playStation(station.url)).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::Next;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e2{};
    e2.status = common::PlaybackStatus::Playing;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc13_buttonEvent_next_nextStation_playStation_Error) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));
    const common::StationData station = {.id = "id", .name = "name", .url = "url"};
    EXPECT_CALL(*mockStationRepo, nextStation()).WillOnce(ReturnRef(station));
    EXPECT_CALL(*mockPlayerService, playStation(station.url)).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::Next;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e2{};
    e2.status = common::PlaybackStatus::Error;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc14_buttonEvent_prev_stop_prevStation_playStation) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Playing));
    EXPECT_CALL(*mockPlayerService, stop()).WillOnce(Return(true));
    const common::StationData station = {.id = "id", .name = "name", .url = "url"};
    EXPECT_CALL(*mockStationRepo, prevStation()).WillOnce(ReturnRef(station));
    EXPECT_CALL(*mockPlayerService, playStation(station.url)).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::Previous;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e2{};
    e2.status = common::PlaybackStatus::Playing;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc15_buttonEvent_prev_nextStation_playStation) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));
    const common::StationData station = {.id = "id", .name = "name", .url = "url"};
    EXPECT_CALL(*mockStationRepo, prevStation()).WillOnce(ReturnRef(station));
    EXPECT_CALL(*mockPlayerService, playStation(station.url)).WillOnce(Return(true));
    common::ButtonPressedEvent e{};
    e.button = common::Button::Previous;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e2{};
    e2.status = common::PlaybackStatus::Playing;
    appController->onEvent(e2);
}

TEST_F(AppControllerTest, tc16_volEvent_setVolume) {
    EXPECT_CALL(*mockPlayerService, setVolume(50)).Times(1);
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));

    common::VolumeChangedEvent e{};
    e.volume = 50;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc17_batteryEvent_forwardedToUi) {
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));

    common::BatteryLevelUpdateEvent e{};
    e.millivolts = 3850;
    e.percent = 61;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc18_lightEvent_notForwardedToUi) {
    EXPECT_CALL(*mockInputService, setMode(false)).Times(1);
    common::LightLevelUpdateEvent e{};
    e.lux = 120;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc19_longPressPlayStop_syncStations_success) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(ManifestTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(ManifestPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestPath, ManifestBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestTmpPath, ManifestPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockStationRepo, load()).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::CurrentStationChangedEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc20_longPressPlayStop_syncStations_restoreFromBackup) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsBackupPath, StationsPath))
        .WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, removeFile(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc21_longPressPlayStop_syncStations_wifiNotConnected) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(false));
    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc22_longPressPlayStop_syncStations_playing) {
    EXPECT_CALL(*mockSensorService, setPlaybackActive(false)).Times(1);
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Playing));
    EXPECT_CALL(*mockPlayerService, stop()).WillOnce(Return(true));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
        EXPECT_NE(e, nullptr);
        return true;
    });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(ManifestTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(ManifestPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestPath, ManifestBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestTmpPath, ManifestPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockStationRepo, load()).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::CurrentStationChangedEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::PlaybackStatusChangedEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::PlaybackStatusChangedEvent p{};
    p.status = common::PlaybackStatus::Stopped;
    appController->onEvent(p);
}

TEST_F(AppControllerTest, tc23_longPressPlayStop_syncStations_stopPlayback_error) {
    EXPECT_CALL(*mockSensorService, setPlaybackActive(false)).Times(1);
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Playing));
    EXPECT_CALL(*mockPlayerService, stop()).WillOnce(Return(true));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
        EXPECT_NE(e, nullptr);
        return true;
    });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::PlaybackStatusChangedEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::PlaybackStatusChangedEvent p{};
    p.status = common::PlaybackStatus::Error;
    appController->onEvent(p);
}

TEST_F(AppControllerTest, tc24_longPressPlayStop_syncStations_downloadFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc25_longPressPlayStop_syncStations_parseManifestRemoteFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc26_longPressPlayStop_syncStations_readFileFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _)).WillOnce(Return(false));

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(ManifestTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(ManifestPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestPath, ManifestBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestTmpPath, ManifestPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockStationRepo, load()).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::CurrentStationChangedEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc27_longPressPlayStop_syncStations_parseManifestLocalFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _)).WillOnce(Return(false));

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(ManifestTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(ManifestPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestPath, ManifestBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestTmpPath, ManifestPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockStationRepo, load()).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::CurrentStationChangedEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc28_longPressPlayStop_syncStations_downloadStationsFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc29_longPressPlayStop_syncStations_parseStationsFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc30_longPressPlayStop_syncStations_writeFileStationsTmpFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc31_longPressPlayStop_syncStations_writeFileManifestTmpFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc32_longPressPlayStop_syncStations_existsStationsFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc33_longPressPlayStop_syncStations_removeFileStationsBackupFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath)).WillOnce(Return(false));

    EXPECT_CALL(*mockFileSystem, removeFile(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc34_longPressPlayStop_syncStations_noLive_renameFileStationsFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(false));

    EXPECT_CALL(*mockFileSystem, removeFile(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc35_longPressPlayStop_syncStations_renameFileStationsBackupFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(false));

    EXPECT_CALL(*mockFileSystem, removeFile(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc36_longPressPlayStop_syncStations_renameFileTmpFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsBackupPath, StationsPath))
        .WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, removeFile(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc37_longPressPlayStop_syncStations_removeFileRemoveBackupFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, removeFile(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc38_longPressPlayStop_syncStations_loadFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(ManifestTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(ManifestPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestPath, ManifestBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(ManifestTmpPath, ManifestPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockStationRepo, load()).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc39_longPressPlayStop_syncStations_existsManifestTmpFail) {
    EXPECT_CALL(*mockPlayerService, getStatus()).WillOnce(Return(common::PlaybackStatus::Stopped));

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, download(ManifestUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = ManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(ManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.1";
            return true;
        });
    EXPECT_CALL(*mockFileSystem, readFile("manifest.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            (void)relativePath;
            outData = LocalManifestJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseManifest(LocalManifestJson, _))
        .WillOnce([](const std::string& serialized, common::ManifestData& outManifest) {
            (void)serialized;
            outManifest.version = "1.0.0";
            return true;
        });

    EXPECT_CALL(*mockHttpClient, download(StationsUrl, _, _))
        .WillOnce([](const std::string& url, std::string& result, const uint32_t& timeoutMs) {
            (void)url;
            (void)timeoutMs;
            result = StationsJson;
            return true;
        });
    EXPECT_CALL(*mockJsonParser, parseStations(StationsJson, _))
        .WillOnce([](const std::string& serialized, std::vector<common::StationData>& outStations) {
            (void)serialized;

            outStations = {{"id1", "station1", "https://example.com"}};
            return true;
        });

    EXPECT_CALL(*mockFileSystem, writeFile(StationsTmpPath, StationsJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, writeFile(ManifestTmpPath, ManifestJson)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsTmpPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(StationsBackupPath))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(StationsPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsPath, StationsBackupPath))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, renameFile(StationsTmpPath, StationsPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(ManifestTmpPath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(ManifestTmpPath)).WillOnce(Return(true));

    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent e{};
    e.button = common::Button::PlayStop;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc40_longPressPlayStop_noPlaybackCmd) {
    EXPECT_CALL(*mockPlayerService, getStatus())
        .WillRepeatedly(Return(common::PlaybackStatus::Stopped));
    EXPECT_CALL(*mockStationRepo, currentStation()).Times(0);

    EXPECT_CALL(*mockWifiService, isConnected()).WillOnce(Return(false));
    EXPECT_CALL(*mockEventQueue, post(_))
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToSyncInProgressScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        })
        .WillOnce([](const common::AppEvent& event) {
            const auto* e = std::get_if<common::SwitchToMainScreenEvent>(&event);
            EXPECT_NE(e, nullptr);
            return true;
        });

    common::ButtonLongPressedEvent start{};
    start.button = common::Button::PlayStop;
    appController->onEvent(start);
}
