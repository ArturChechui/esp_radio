#include "UiServiceTest.hpp"

#include <cstring>

#include "Events.hpp"
#include "Fonts.hpp"
#include "Types.hpp"

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Ref;

static constexpr size_t FRAMEBUFFER_SIZE = 1024U;  // bytes. 128x64 / 8

void UiServiceTest::SetUp() {
    mockDisplay = std::make_unique<adapters::MockDisplay>();
    mockRepo = std::make_unique<services::MockStationRepository>();

    uiService = std::make_unique<services::UiService>(*mockDisplay, *mockRepo);
}

void UiServiceTest::TearDown() {
    uiService.reset();

    mockDisplay.reset();
    mockRepo.reset();
}

TEST_F(UiServiceTest, tc01_onEvent_renderStations_fetchesFromRepoAndDisplays) {
    // Preparation
    std::vector<common::StationData> stations = {{"id1", "S1", "url1"}, {"id2", "S2", "url2"}};
    std::vector<uint8_t> expectedFrameBuffer(FRAMEBUFFER_SIZE, 0);

    // TODO: make a copy of the src for filling or leave as is?
    // after status bar + indicator
    const uint16_t page1base = (8U / 8U) * 128U + 6U;
    // after status bar + after station 1 + indicator
    const uint16_t page2base = (16U / 8U) * 128U + 6U;
    expectedFrameBuffer[page1base + 0] = common::FONT5x7[static_cast<uint8_t>('S')][0];
    expectedFrameBuffer[page1base + 1] = common::FONT5x7[static_cast<uint8_t>('S')][1];
    expectedFrameBuffer[page1base + 2] = common::FONT5x7[static_cast<uint8_t>('S')][2];
    expectedFrameBuffer[page1base + 3] = common::FONT5x7[static_cast<uint8_t>('S')][3];
    expectedFrameBuffer[page1base + 4] = common::FONT5x7[static_cast<uint8_t>('S')][4];
    expectedFrameBuffer[page1base + 5] = 0x00;  // 1px spacing
    expectedFrameBuffer[page1base + 6] = common::FONT5x7[static_cast<uint8_t>('1')][0];
    expectedFrameBuffer[page1base + 7] = common::FONT5x7[static_cast<uint8_t>('1')][1];
    expectedFrameBuffer[page1base + 8] = common::FONT5x7[static_cast<uint8_t>('1')][2];
    expectedFrameBuffer[page1base + 9] = common::FONT5x7[static_cast<uint8_t>('1')][3];
    expectedFrameBuffer[page1base + 10] = common::FONT5x7[static_cast<uint8_t>('1')][4];
    expectedFrameBuffer[page1base + 11] = 0x00;  // 1px spacing
    expectedFrameBuffer[page2base + 0] = common::FONT5x7[static_cast<uint8_t>('S')][0];
    expectedFrameBuffer[page2base + 1] = common::FONT5x7[static_cast<uint8_t>('S')][1];
    expectedFrameBuffer[page2base + 2] = common::FONT5x7[static_cast<uint8_t>('S')][2];
    expectedFrameBuffer[page2base + 3] = common::FONT5x7[static_cast<uint8_t>('S')][3];
    expectedFrameBuffer[page2base + 4] = common::FONT5x7[static_cast<uint8_t>('S')][4];
    expectedFrameBuffer[page2base + 5] = 0x00;  // 1px spacing
    expectedFrameBuffer[page2base + 6] = common::FONT5x7[static_cast<uint8_t>('2')][0];
    expectedFrameBuffer[page2base + 7] = common::FONT5x7[static_cast<uint8_t>('2')][1];
    expectedFrameBuffer[page2base + 8] = common::FONT5x7[static_cast<uint8_t>('2')][2];
    expectedFrameBuffer[page2base + 9] = common::FONT5x7[static_cast<uint8_t>('2')][3];
    expectedFrameBuffer[page2base + 10] = common::FONT5x7[static_cast<uint8_t>('2')][4];
    expectedFrameBuffer[page2base + 11] = 0x00;  // 1px spacing

    common::UiRenderEvent event;
    event.renderType = common::RenderType::Stations;
    event.selectedStationIndex = 1;

    // Expectations
    EXPECT_CALL(*mockRepo, getStations()).WillOnce(::testing::ReturnRef(stations));
    EXPECT_CALL(*mockDisplay, showFramebuffer(_, expectedFrameBuffer.size()))
        .Times(1)
        .WillOnce([&expectedFrameBuffer](const uint8_t* framebuffer, const size_t& len) {
            EXPECT_EQ(expectedFrameBuffer.size(), len);
            EXPECT_TRUE(std::memcmp(framebuffer, expectedFrameBuffer.data(), len) == 0);

            return true;
        });

    // Act
    uiService->onEvent(event);
}
