#include <cstring>

#include "DisplayTest.hpp"

using ::testing::_;
using ::testing::Return;

static constexpr uint8_t OLED_I2C_ADDR = 0x3C;
static constexpr uint8_t DATA_BYTE = 0xFF;
static constexpr uint8_t DATA_CTRL_BYTE = 0x40;
static constexpr uint8_t CMD_CTRL_BYTE = 0x00;
static constexpr size_t FRAMEBUFFER_SIZE = 1024U;  // bytes. 128x64 / 8

void DisplayTest::SetUp() {
    display = std::make_unique<adapters::Display>(mockI2cBus);
}

void DisplayTest::TearDown() {
    display.reset();
}

void DisplayTest::initDisplay() {
    const std::vector<uint8_t> expectedBytes = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00,
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x20, 0xA4, 0xA6, 0xAF};

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, expectedBytes.size(), _))
        .Times(1)
        .WillOnce([expectedBytes](const uint8_t& addr, const uint8_t* data, const size_t& len,
                                  const uint32_t& timeout) {
            EXPECT_EQ(expectedBytes.size(), len);
            EXPECT_TRUE(std::memcmp(data, expectedBytes.data(), len) == 0);
            return true;
        });
    ASSERT_EQ(true, display->init());
}

TEST_F(DisplayTest, tc01_init_success) {
    initDisplay();
}

TEST_F(DisplayTest, tc02_showFramebuffer_success) {
    initDisplay();

    // Preparation
    const std::vector<uint8_t> framebuffer(FRAMEBUFFER_SIZE, DATA_BYTE);
    std::vector<uint8_t> expectedFrameBuffer(FRAMEBUFFER_SIZE + 1U);
    expectedFrameBuffer[0] = DATA_CTRL_BYTE;
    std::fill(expectedFrameBuffer.begin() + 1U, expectedFrameBuffer.end(), DATA_BYTE);
    const std::vector<uint8_t> pageCmdBytes = {CMD_CTRL_BYTE, 0x22, 0x00, 0x07};
    const std::vector<uint8_t> colCmdBytes = {CMD_CTRL_BYTE, 0x21, 0x00, 0x7F};

    // Expectations
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, _, _))
        .Times(2)
        .WillRepeatedly([pageCmdBytes, colCmdBytes](const uint8_t& addr, const uint8_t* data,
                                                    const size_t& len, const uint32_t& timeout) {
            EXPECT_TRUE(pageCmdBytes.size() == len || colCmdBytes.size() == len);
            EXPECT_TRUE(std::memcmp(data, pageCmdBytes.data(), len) == 0 ||
                        std::memcmp(data, colCmdBytes.data(), len) == 0);
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, expectedFrameBuffer.size(), _))
        .Times(1)
        .WillOnce([expectedFrameBuffer](const uint8_t& addr, const uint8_t* data, const size_t& len,
                                        const uint32_t& timeout) {
            EXPECT_EQ(expectedFrameBuffer.size(), len);
            EXPECT_TRUE(std::memcmp(data, expectedFrameBuffer.data(), len) == 0);
            return true;
        });

    // Execution
    display->showFramebuffer(framebuffer.data(), framebuffer.size());
}

TEST_F(DisplayTest, tc03_showFramebuffer_notReady) {
    const std::vector<uint8_t> framebuffer(FRAMEBUFFER_SIZE, DATA_BYTE);

    // No expectations on I2C bus since display is not initialized
    display->showFramebuffer(framebuffer.data(), framebuffer.size());
}
