#include "AppControllerTest.hpp"

#include "Events.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

void AppControllerTest::SetUp() {
    mockEventQueue = std::make_unique<common::MockEventQueue>();
    mockPlayerService = std::make_unique<services::MockPlayerService>();
    mockStationRepo = std::make_unique<services::MockStationRepository>();

    appController = std::make_unique<core::AppController>(*mockPlayerService, *mockStationRepo,
                                                          *mockEventQueue);
}

void AppControllerTest::TearDown() {
    appController.reset();

    mockPlayerService.reset();
    mockStationRepo.reset();
    mockEventQueue.reset();
}

TEST_F(AppControllerTest, tc01_init_success) {
    EXPECT_TRUE(appController->init());
}

TEST_F(AppControllerTest, tc02_readyEvent) {
    EXPECT_TRUE(appController->init());

    common::SystemReadyEvent e{};
    e.showSplashScreen = true;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc03_tempEvent) {
    EXPECT_TRUE(appController->init());

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::TempHumidUpdateEvent e{};
    e.humidity = 45;
    e.temperature = 11.0;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc04_wifiEvent) {
    EXPECT_TRUE(appController->init());

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::WifiStateChangedEvent e{};
    e.bars = 3;
    e.isConnected = true;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc05_playbackStatusEvent) {
    EXPECT_TRUE(appController->init());

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    common::PlaybackStatusChangedEvent e{};
    e.status = common::PlaybackStatus::Playing;
    appController->onEvent(e);
}

TEST_F(AppControllerTest, tc06_buttonEvent_playStop_playStation_buffering) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc07_buttonEvent_playStop_playStation_playing) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc08_buttonEvent_playStop_stop) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc09_buttonEvent_next_stop_nextStation_playStation) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc10_buttonEvent_next_nextStation_playStation) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc11_buttonEvent_next_nextStation_playStation_Error) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc12_buttonEvent_prev_stop_prevStation_playStation) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc13_buttonEvent_prev_nextStation_playStation) {
    EXPECT_TRUE(appController->init());

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

TEST_F(AppControllerTest, tc14_volEvent_setVolume) {
    EXPECT_TRUE(appController->init());

    EXPECT_CALL(*mockPlayerService, setVolume(50)).Times(1);
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));

    common::VolumeChangedEvent e{};
    e.volume = 50;
    appController->onEvent(e);
}