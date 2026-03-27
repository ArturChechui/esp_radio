#include "UiServiceTest.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "Events.hpp"
#include "Fonts.hpp"
#include "Types.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

// Geometry (must match UiService.cpp)
constexpr uint8_t kWidth = 128;
constexpr uint8_t kHeight = 64;
constexpr uint8_t kPages = kHeight / 8;
constexpr size_t kFramebufferSize = static_cast<size_t>(kWidth) * kPages;  // 1024

// Layout (must match UiService.cpp)
constexpr common::Rect kStatusTextRect{0U, 0U, 80U, 16U};
constexpr common::Rect kStatusVolRect{80U, 0U, 16U, 16U};
constexpr common::Rect kStatusBatRect{96U, 0U, 16U, 16U};
constexpr common::Rect kStatusWifiRect{112U, 0U, 16U, 16U};

constexpr common::Rect kMainIconRect{0U, 16U, common::fonts::Icon48W, common::fonts::Icon48H};
constexpr common::Rect kMainTextRect{static_cast<uint8_t>(common::fonts::Icon48W + 4U), 16U,
                                     static_cast<uint8_t>(128U - (common::fonts::Icon48W + 4U)),
                                     48U};

struct Window {
    uint8_t col0, col1, page0, page1;
    size_t len;
};

inline Window rectToWindow(const common::Rect& r) {
    const uint16_t x0 = r.x;
    const uint16_t y0 = r.y;
    const uint16_t x1 = std::min<uint16_t>(static_cast<uint16_t>(r.x) + r.w, kWidth);
    const uint16_t y1 = std::min<uint16_t>(static_cast<uint16_t>(r.y) + r.h, kHeight);

    Window w{};
    w.col0 = static_cast<uint8_t>(x0);
    w.col1 = static_cast<uint8_t>(x1 - 1);
    w.page0 = static_cast<uint8_t>(y0 / 8);
    w.page1 = static_cast<uint8_t>((y1 - 1) / 8);

    const size_t widthBytes = static_cast<size_t>(x1 - x0);
    const size_t pagesCnt = static_cast<size_t>(w.page1 - w.page0 + 1);
    w.len = widthBytes * pagesCnt;
    return w;
}

inline std::vector<uint8_t> packFromFramebuffer(const std::vector<uint8_t>& fb, uint8_t col0,
                                                uint8_t col1, uint8_t page0, uint8_t page1) {
    const size_t widthBytes = static_cast<size_t>(col1 - col0 + 1);
    const size_t pagesCnt = static_cast<size_t>(page1 - page0 + 1);

    std::vector<uint8_t> out;
    out.resize(widthBytes * pagesCnt);

    size_t off = 0;
    for (uint8_t p = page0; p <= page1; ++p) {
        const size_t base = static_cast<size_t>(p) * kWidth + col0;
        std::memcpy(out.data() + off, fb.data() + base, widthBytes);
        off += widthBytes;
    }
    return out;
}

}  // namespace

void UiServiceTest::SetUp() {
    mockDisplay = std::make_unique<StrictMock<adapters::MockDisplay>>();
    mockRepo = std::make_unique<StrictMock<services::MockStationRepository>>();

    uiService = std::make_unique<services::UiService>(*mockDisplay, *mockRepo);
}

void UiServiceTest::TearDown() {
    uiService.reset();

    mockDisplay.reset();
    mockRepo.reset();
}

void UiServiceTest::initSuccess() {
    static const common::StationData station{"id", "AB", "url"};
    EXPECT_CALL(*mockRepo, currentStation()).WillRepeatedly(ReturnRef(station));

    EXPECT_CALL(*mockDisplay, showFramebuffer(_, _))
        .WillOnce([&](const uint8_t* fb, const size_t len) {
            EXPECT_EQ(len, kFramebufferSize);
            const auto& v = uiService->getFramebuffer();
            EXPECT_EQ(fb, v.data());
            EXPECT_TRUE(std::any_of(v.begin(), v.end(), [](uint8_t b) { return b != 0; }));
            return true;
        });

    EXPECT_TRUE(uiService->init());
}

void UiServiceTest::switchToMainScreenSuccess() {
    EXPECT_CALL(*mockDisplay, showFramebuffer(_, _)).WillOnce(Return(true));
    common::SwitchToMainScreenEvent e{};
    uiService->onEvent(e);
}

void UiServiceTest::switchToWifiProvScreenSuccess() {
    EXPECT_CALL(*mockDisplay, showFramebuffer(_, _)).WillOnce(Return(true));
    common::SwitchToWifiProvScreenEvent e{};
    uiService->onEvent(e);
}

void UiServiceTest::switchToSyncInProgressScreenSuccess() {
    EXPECT_CALL(*mockDisplay, showFramebuffer(_, _)).WillOnce(Return(true));
    common::SwitchToSyncInProgressScreenEvent e{};
    uiService->onEvent(e);
}

TEST_F(UiServiceTest, tc01_init_success) {
    initSuccess();
}

TEST_F(UiServiceTest, tc02_init_fail) {
    static const common::StationData station{"id", "AB", "url"};
    EXPECT_CALL(*mockRepo, currentStation()).WillRepeatedly(ReturnRef(station));
    EXPECT_CALL(*mockDisplay, showFramebuffer(_, _)).WillOnce(Return(false));
    EXPECT_FALSE(uiService->init());
}

TEST_F(UiServiceTest, tc03_stationEvent_partialFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    static const common::StationData station{"id", "AB", "url"};
    EXPECT_CALL(*mockRepo, currentStation()).WillOnce(ReturnRef(station));

    const auto win = rectToWindow(kMainTextRect);

    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);

            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(expected.size(), len);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });

    uiService->onEvent(common::CurrentStationChangedEvent{});

    const uint8_t x = kMainTextRect.x;
    const uint8_t page0 = 2;
    const auto* glyphA = common::fonts::mainGlyph('A');
    ASSERT_NE(glyphA, nullptr);

    for (uint8_t p = 0; p < common::fonts::MainPages; ++p) {
        const size_t fbBase = static_cast<size_t>(page0 + p) * kWidth + x;
        const size_t srcBase = static_cast<size_t>(p) * common::fonts::MainW;
        EXPECT_EQ(0, std::memcmp(uiService->getFramebuffer().data() + fbBase, glyphA + srcBase,
                                 common::fonts::MainW));
    }
}

TEST_F(UiServiceTest, tc04_wifiEvent_partialFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    const auto win = rectToWindow(kStatusWifiRect);

    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);
            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });

    common::WifiStateChangedEvent e{};
    e.isConnected = true;
    e.bars = 3;
    uiService->onEvent(e);
}

TEST_F(UiServiceTest, tc05_wifiEvent_noFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    common::WifiStateChangedEvent e{};
    e.isConnected = false;
    e.bars = 0;
    uiService->onEvent(e);
}

TEST_F(UiServiceTest, tc06_tempEvent_partialFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    const auto win = rectToWindow(kStatusTextRect);

    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);
            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });

    common::TempHumidUpdateEvent t{};
    t.temperature = 12;
    t.humidity = 34;
    uiService->onEvent(t);
}

TEST_F(UiServiceTest, tc07_playbackEvent_partialFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    const auto win = rectToWindow(kMainIconRect);

    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);
            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });

    common::PlaybackStatusChangedEvent p{};
    p.status = common::PlaybackStatus::Playing;
    uiService->onEvent(p);
}

TEST_F(UiServiceTest, tc08_wifiEvent_partialFlush_fail_tempEvent_fullFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    const auto win = rectToWindow(kStatusWifiRect);
    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce(Return(false));
    common::WifiStateChangedEvent e{};
    e.isConnected = true;
    e.bars = 2;
    uiService->onEvent(e);

    EXPECT_CALL(*mockDisplay, showFramebuffer(_, _)).WillOnce(Return(true));
    common::TempHumidUpdateEvent t{};
    t.temperature = 20;
    t.humidity = 50;
    uiService->onEvent(t);
}

TEST_F(UiServiceTest, tc09_volEvent_partialFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    const auto win = rectToWindow(kStatusVolRect);

    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);
            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });

    common::VolumeChangedEvent e{};
    e.volume = 70;
    uiService->onEvent(e);
}

TEST_F(UiServiceTest, tc10_switchToWifiProv_switchToMain_volEvent) {
    initSuccess();
    switchToWifiProvScreenSuccess();
    switchToMainScreenSuccess();

    const auto win = rectToWindow(kStatusVolRect);

    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);
            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });

    common::VolumeChangedEvent e{};
    e.volume = 70;
    uiService->onEvent(e);
}

TEST_F(UiServiceTest,
       tc11_switchToWifiProv_events_switchToMain_sameEvents_noFlush_wifiEvent_partialFlush) {
    initSuccess();
    switchToWifiProvScreenSuccess();

    common::VolumeChangedEvent v{};
    v.volume = 70;
    uiService->onEvent(v);
    common::TempHumidUpdateEvent t{};
    t.temperature = 20;
    t.humidity = 50;
    uiService->onEvent(t);
    common::WifiStateChangedEvent w{};
    w.isConnected = true;
    w.bars = 2;
    uiService->onEvent(w);
    common::PlaybackStatusChangedEvent p{};
    p.status = common::PlaybackStatus::Playing;
    uiService->onEvent(p);
    common::CurrentStationChangedEvent s{};
    uiService->onEvent(s);

    switchToMainScreenSuccess();

    common::VolumeChangedEvent v2{};
    v2.volume = 70;
    uiService->onEvent(v2);
    common::TempHumidUpdateEvent t2{};
    t2.temperature = 20;
    t2.humidity = 50;
    uiService->onEvent(t2);
    common::WifiStateChangedEvent w2{};
    w2.isConnected = true;
    w2.bars = 2;
    uiService->onEvent(w2);
    common::PlaybackStatusChangedEvent p2{};
    p2.status = common::PlaybackStatus::Playing;
    uiService->onEvent(p2);

    const auto win = rectToWindow(kStatusWifiRect);
    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);
            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });
    common::WifiStateChangedEvent w3{};
    w3.isConnected = true;
    w3.bars = 4;
    uiService->onEvent(w3);
}

TEST_F(UiServiceTest, tc12_batteryEvent_partialFlush) {
    initSuccess();
    switchToMainScreenSuccess();

    const auto win = rectToWindow(kStatusBatRect);

    EXPECT_CALL(*mockDisplay, showWindow(win.col0, win.col1, win.page0, win.page1, _, _))
        .WillOnce([&](const uint8_t col0, const uint8_t col1, const uint8_t page0,
                      const uint8_t page1, const uint8_t* data, const size_t len) {
            EXPECT_EQ(len, win.len);
            const auto expected =
                packFromFramebuffer(uiService->getFramebuffer(), col0, col1, page0, page1);
            EXPECT_EQ(0, std::memcmp(data, expected.data(), len));
            return true;
        });

    common::BatteryLevelUpdateEvent e{};
    e.millivolts = 3720;
    e.percent = 18;
    uiService->onEvent(e);
}

TEST_F(UiServiceTest, tc13_switchToSyncInProgress_success) {
    initSuccess();
    switchToSyncInProgressScreenSuccess();
}

TEST_F(UiServiceTest, tc14_syncInProgress_noPartialUpdates) {
    initSuccess();
    switchToSyncInProgressScreenSuccess();

    common::VolumeChangedEvent v{};
    v.volume = 70;
    uiService->onEvent(v);

    common::WifiStateChangedEvent w{};
    w.isConnected = true;
    w.bars = 3;
    uiService->onEvent(w);

    common::PlaybackStatusChangedEvent p{};
    p.status = common::PlaybackStatus::Playing;
    uiService->onEvent(p);
}
