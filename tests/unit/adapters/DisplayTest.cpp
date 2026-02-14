#include "DisplayTest.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "Display.hpp"

// Adjust include to your mock
#include "MockI2cBus.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {
constexpr uint8_t OLED_I2C_ADDR = 0x3C;
constexpr uint8_t DATA_CTRL_BYTE = 0x40;
constexpr uint8_t CMD_CTRL_BYTE = 0x00;

constexpr size_t WIDTH = 128;
constexpr size_t HEIGHT = 64;
constexpr size_t FRAMEBUFFER_SIZE = (WIDTH * HEIGHT) / 8;  // 1024

// Must match Display.cpp ChunkBytes
constexpr size_t CHUNK = 16;

std::vector<uint8_t> initCmd() {
    return {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1,
            0xC8, 0xDA, 0x12, 0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x20, 0xA4, 0xA6, 0xAF};
}

static std::vector<uint8_t> withCtrl(const uint8_t ctrl, const uint8_t* p, const size_t n) {
    std::vector<uint8_t> out;
    out.resize(1 + n);
    out[0] = ctrl;
    std::memcpy(out.data() + 1, p, n);
    return out;
}

static bool bytesEq(const uint8_t* data, size_t len, const std::vector<uint8_t>& expected) {
    if (len != expected.size())
        return false;
    return std::memcmp(data, expected.data(), len) == 0;
}
}  // namespace

void DisplayTest::SetUp() {
    display = std::make_unique<adapters::Display>(mockI2cBus);
}

void DisplayTest::TearDown() {
    display.reset();
}

void DisplayTest::initOkExpectations() {
    const auto cmd = initCmd();
    const auto exp1 = withCtrl(CMD_CTRL_BYTE, cmd.data(), 16);
    const auto exp2 = withCtrl(CMD_CTRL_BYTE, cmd.data() + 16, 9);

    // Init is 25 bytes command -> writeCommand chunks it into 16 + 9
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, 1 + 16, _))
        .WillOnce([&](const uint8_t addr, const uint8_t* data, const size_t len, const uint32_t) {
            EXPECT_EQ(addr, OLED_I2C_ADDR);
            EXPECT_TRUE(bytesEq(data, len, exp1));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, 1 + 9, _))
        .WillOnce([&](const uint8_t addr, const uint8_t* data, const size_t len, const uint32_t) {
            EXPECT_EQ(addr, OLED_I2C_ADDR);
            EXPECT_TRUE(bytesEq(data, len, exp2));
            return true;
        });

    ASSERT_TRUE(display->init());
}

TEST_F(DisplayTest, tc01_init_success_sends_init_sequence_chunked) {
    initOkExpectations();
}

TEST_F(DisplayTest, tc02_init_fail_if_first_chunk_write_fails) {
    // first chunk fails => init returns false
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, 1 + 16, _)).WillOnce(Return(false));

    EXPECT_FALSE(display->init());
}

TEST_F(DisplayTest, tc03_showFramebuffer_returns_false_when_not_initialized) {
    std::vector<uint8_t> fb(FRAMEBUFFER_SIZE, 0xFF);

    // No I2C calls expected
    EXPECT_FALSE(display->showFramebuffer(fb.data(), fb.size()));
}

TEST_F(DisplayTest, tc04_showWindow_returns_false_for_invalid_ranges) {
    initOkExpectations();

    std::vector<uint8_t> data(1, 0xAA);

    // col0>col1
    EXPECT_FALSE(display->showWindow(10, 9, 0, 0, data.data(), data.size()));
    // page0>page1
    EXPECT_FALSE(display->showWindow(0, 0, 2, 1, data.data(), data.size()));
}

TEST_F(DisplayTest, tc05_showWindow_returns_false_on_len_mismatch) {
    initOkExpectations();

    // col 0..1 => width=2, page 0..0 => pages=1 => expected=2
    std::vector<uint8_t> data(1, 0xAA);
    EXPECT_FALSE(display->showWindow(0, 1, 0, 0, data.data(), data.size()));
}

TEST_F(DisplayTest, tc06_showFramebuffer_success_sends_col_page_then_data_in_chunks) {
    InSequence seq;
    initOkExpectations();

    std::vector<uint8_t> fb(FRAMEBUFFER_SIZE, 0xAB);

    const std::vector<uint8_t> colCmd = {0x21, 0x00, 0x7F};
    const std::vector<uint8_t> pageCmd = {0x22, 0x00, 0x07};
    const auto exp1 = withCtrl(CMD_CTRL_BYTE, colCmd.data(), colCmd.size());
    const auto exp2 = withCtrl(CMD_CTRL_BYTE, pageCmd.data(), pageCmd.size());

    // Expect column+page commands (order matters: col then page in your code)
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp1.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* data, size_t len, uint32_t) {
            EXPECT_TRUE(bytesEq(data, len, exp1));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp2.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* data, size_t len, uint32_t) {
            EXPECT_TRUE(bytesEq(data, len, exp2));
            return true;
        });

    // Data: 1024 bytes -> chunked into 16-byte chunks => 64 calls
    // Each call sends 1 + chunk bytes with DATA_CTRL_BYTE at [0]
    for (size_t offset = 0; offset < fb.size(); offset += CHUNK) {
        const size_t chunk = std::min(CHUNK, fb.size() - offset);
        const auto expected = withCtrl(DATA_CTRL_BYTE, fb.data() + offset, chunk);

        EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, expected.size(), _))
            .WillOnce([expected](uint8_t, const uint8_t* data, size_t len, uint32_t) {
                EXPECT_TRUE(bytesEq(data, len, expected));
                return true;
            });
    }

    EXPECT_TRUE(display->showFramebuffer(fb.data(), fb.size()));
}

TEST_F(DisplayTest, tc07_showWindow_fails_if_column_cmd_fails_no_further_writes) {
    initOkExpectations();

    // window: col 0..0 page 0..0 -> len=1
    uint8_t b = 0x11;

    const std::vector<uint8_t> colCmd = {0x21, 0x00, 0x00};
    const auto exp1 = withCtrl(CMD_CTRL_BYTE, colCmd.data(), colCmd.size());
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp1.size(), _)).WillOnce(Return(false));

    // no page cmd, no data expected
    EXPECT_FALSE(display->showWindow(0, 0, 0, 0, &b, 1));
}

TEST_F(DisplayTest, tc08_showWindow_fails_if_page_cmd_fails_no_data_write) {
    InSequence seq;

    initOkExpectations();

    uint8_t b = 0x11;

    const std::vector<uint8_t> colCmd = {0x21, 0x00, 0x00};
    const std::vector<uint8_t> pageCmd = {0x22, 0x00, 0x00};
    const auto exp1 = withCtrl(CMD_CTRL_BYTE, colCmd.data(), colCmd.size());
    const auto exp2 = withCtrl(CMD_CTRL_BYTE, pageCmd.data(), pageCmd.size());

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp1.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* data, size_t len, uint32_t) {
            EXPECT_TRUE(bytesEq(data, len, exp1));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp2.size(), _)).WillOnce(Return(false));

    EXPECT_FALSE(display->showWindow(0, 0, 0, 0, &b, 1));
}

TEST_F(DisplayTest, tc09_showWindow_fails_if_any_data_chunk_fails) {
    InSequence seq;

    initOkExpectations();

    // window: col 0..31 (32 cols), page 0..0 (1 page) => len=32
    std::vector<uint8_t> data(32, 0x5A);

    const std::vector<uint8_t> colCmd = {0x21, 0x00, 0x1F};
    const std::vector<uint8_t> pageCmd = {0x22, 0x00, 0x00};
    const auto exp1 = withCtrl(CMD_CTRL_BYTE, colCmd.data(), colCmd.size());
    const auto exp2 = withCtrl(CMD_CTRL_BYTE, pageCmd.data(), pageCmd.size());

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp1.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* d, size_t l, uint32_t) {
            EXPECT_TRUE(bytesEq(d, l, exp1));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp2.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* d, size_t l, uint32_t) {
            EXPECT_TRUE(bytesEq(d, l, exp2));
            return true;
        });

    // First data chunk (16) ok, second (16) fails
    const auto expected1 = withCtrl(DATA_CTRL_BYTE, data.data(), 16);
    const auto expected2 = withCtrl(DATA_CTRL_BYTE, data.data() + 16, 16);

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, expected1.size(), _))
        .WillOnce([expected1](uint8_t, const uint8_t* d, size_t l, uint32_t) {
            EXPECT_TRUE(bytesEq(d, l, expected1));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, expected2.size(), _))
        .WillOnce(Return(false));

    EXPECT_FALSE(display->showWindow(0, 31, 0, 0, data.data(), data.size()));
}
