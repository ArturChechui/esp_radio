#include "DisplayTest.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "BoardConfig.hpp"
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
constexpr uint8_t SH1106_COL_OFFSET = common::OLED_COLUMN_OFFSET;

constexpr size_t WIDTH = 128;
constexpr size_t HEIGHT = 64;
constexpr size_t PAGES = HEIGHT / 8;
constexpr size_t FRAMEBUFFER_SIZE = (WIDTH * HEIGHT) / 8;  // 1024

// Must match Display.cpp ChunkBytes
constexpr size_t CHUNK = 16;

std::vector<uint8_t> initCmd() {
    return {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0xAD, 0x8B, 0xA1, 0xC8,
            0xDA, 0x12, 0x81, 0x7F, 0xD9, 0x22, 0xDB, 0x20, 0xA4, 0xA6, 0xAF};
}

static std::array<uint8_t, 3> sh1106PageCmd(const uint8_t page, const uint8_t col0) {
    const uint8_t physical = static_cast<uint8_t>(col0 + SH1106_COL_OFFSET);
    return {
        static_cast<uint8_t>(0xB0 + page),
        static_cast<uint8_t>(0x10 | ((physical >> 4U) & 0x0FU)),
        static_cast<uint8_t>(physical & 0x0FU),
    };
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
    const auto exp2 = withCtrl(CMD_CTRL_BYTE, cmd.data() + 16, 7);

    // Init is 23 bytes command -> writeCommand chunks it into 16 + 7.
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, 1 + 16, _))
        .WillOnce([&](const uint8_t addr, const uint8_t* data, const size_t len, const uint32_t) {
            EXPECT_EQ(addr, OLED_I2C_ADDR);
            EXPECT_TRUE(bytesEq(data, len, exp1));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, 1 + 7, _))
        .WillOnce([&](const uint8_t addr, const uint8_t* data, const size_t len, const uint32_t) {
            EXPECT_EQ(addr, OLED_I2C_ADDR);
            EXPECT_TRUE(bytesEq(data, len, exp2));
            return true;
        });

    ASSERT_TRUE(display->init());
}

TEST_F(DisplayTest, tc01_init_success) {
    initOkExpectations();
}

TEST_F(DisplayTest, tc02_init_initCmdFail) {
    // first chunk fails => init returns false
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, 1 + 16, _)).WillOnce(Return(false));

    EXPECT_FALSE(display->init());
}

TEST_F(DisplayTest, tc03_showFramebuffer_notInited) {
    std::vector<uint8_t> fb(FRAMEBUFFER_SIZE, 0xFF);

    // No I2C calls expected
    EXPECT_FALSE(display->showFramebuffer(fb.data(), fb.size()));
}

TEST_F(DisplayTest, tc04_showWindow_invalidRanges) {
    initOkExpectations();

    std::vector<uint8_t> data(1, 0xAA);

    // col0>col1
    EXPECT_FALSE(display->showWindow(10, 9, 0, 0, data.data(), data.size()));
    // page0>page1
    EXPECT_FALSE(display->showWindow(0, 0, 2, 1, data.data(), data.size()));

    std::vector<uint8_t> wideData(129, 0xAA);
    // col out of bounds for 128px panel.
    EXPECT_FALSE(display->showWindow(0, 128, 0, 0, wideData.data(), wideData.size()));

    std::vector<uint8_t> tallData(9, 0xAA);
    // page out of bounds for 8 pages.
    EXPECT_FALSE(display->showWindow(0, 0, 0, 8, tallData.data(), tallData.size()));
}

TEST_F(DisplayTest, tc05_showWindow_lenMismatch) {
    initOkExpectations();

    // col 0..1 => width=2, page 0..0 => pages=1 => expected=2
    std::vector<uint8_t> data(1, 0xAA);
    EXPECT_FALSE(display->showWindow(0, 1, 0, 0, data.data(), data.size()));
}

TEST_F(DisplayTest, tc06_showFramebuffer_success) {
    InSequence seq;
    initOkExpectations();

    std::vector<uint8_t> fb(FRAMEBUFFER_SIZE, 0xAB);

    for (size_t page = 0; page < PAGES; ++page) {
        const auto cmd = sh1106PageCmd(static_cast<uint8_t>(page), 0U);
        const auto pageCmd = withCtrl(CMD_CTRL_BYTE, cmd.data(), cmd.size());
        EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, pageCmd.size(), _))
            .WillOnce([pageCmd](uint8_t, const uint8_t* data, size_t len, uint32_t) {
                EXPECT_TRUE(bytesEq(data, len, pageCmd));
                return true;
            });

        const size_t pageBase = page * WIDTH;
        for (size_t offset = 0; offset < WIDTH; offset += CHUNK) {
            const size_t chunk = std::min(CHUNK, WIDTH - offset);
            const auto expected = withCtrl(DATA_CTRL_BYTE, fb.data() + pageBase + offset, chunk);

            EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, expected.size(), _))
                .WillOnce([expected](uint8_t, const uint8_t* data, size_t len, uint32_t) {
                    EXPECT_TRUE(bytesEq(data, len, expected));
                    return true;
                });
        }
    }

    EXPECT_TRUE(display->showFramebuffer(fb.data(), fb.size()));
}

TEST_F(DisplayTest, tc07_showWindow_columnCmdFail) {
    initOkExpectations();

    uint8_t b = 0x11;

    const auto cmd = sh1106PageCmd(0, 0);
    const auto exp = withCtrl(CMD_CTRL_BYTE, cmd.data(), cmd.size());
    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp.size(), _)).WillOnce(Return(false));

    EXPECT_FALSE(display->showWindow(0, 0, 0, 0, &b, 1));
}

TEST_F(DisplayTest, tc08_showWindow_pageCmdFail) {
    InSequence seq;

    initOkExpectations();

    std::array<uint8_t, 2> data = {0x11, 0x22};

    const auto page0CmdBytes = sh1106PageCmd(0, 0);
    const auto page1CmdBytes = sh1106PageCmd(1, 0);
    const auto page0Cmd = withCtrl(CMD_CTRL_BYTE, page0CmdBytes.data(), page0CmdBytes.size());
    const auto page1Cmd = withCtrl(CMD_CTRL_BYTE, page1CmdBytes.data(), page1CmdBytes.size());
    const auto page0Data = withCtrl(DATA_CTRL_BYTE, data.data(), 1);

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, page0Cmd.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* data, size_t len, uint32_t) {
            EXPECT_TRUE(bytesEq(data, len, page0Cmd));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, page0Data.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* d, size_t l, uint32_t) {
            EXPECT_TRUE(bytesEq(d, l, page0Data));
            return true;
        });

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, page1Cmd.size(), _)).WillOnce(Return(false));

    EXPECT_FALSE(display->showWindow(0, 0, 0, 1, data.data(), data.size()));
}

TEST_F(DisplayTest, tc09_showWindow_dataChunkFail) {
    InSequence seq;

    initOkExpectations();

    // window: col 0..31 (32 cols), page 0..0 (1 page) => len=32
    std::vector<uint8_t> data(32, 0x5A);

    const auto cmd = sh1106PageCmd(0, 0);
    const auto exp = withCtrl(CMD_CTRL_BYTE, cmd.data(), cmd.size());

    EXPECT_CALL(mockI2cBus, writeBytes(OLED_I2C_ADDR, _, exp.size(), _))
        .WillOnce([&](uint8_t, const uint8_t* d, size_t l, uint32_t) {
            EXPECT_TRUE(bytesEq(d, l, exp));
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
