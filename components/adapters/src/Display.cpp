#include "Display.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "BoardConfig.hpp"

// IDF
#include <esp_log.h>

namespace adapters {
namespace {
constexpr const char* Tag = "Display";

constexpr uint8_t ControlByteCommand = 0x00;
constexpr uint8_t ControlByteData = 0x40;
constexpr size_t ChunkBytes = 16;  // TODO: try 32/64.
constexpr uint8_t DisplayWidth = 128U;
constexpr uint8_t DisplayPages = 8U;

// SH1106 init sequence (128x64).
constexpr std::array<uint8_t, 23> InitCmd = {
    0xAE,        // Display OFF
    0xD5, 0x80,  // Clock
    0xA8, 0x3F,  // MUX
    0xD3, 0x00,  // Offset
    0x40,        // Start line
    0xAD, 0x8B,  // DCDC ON
    0xA1,        // Segment remap
    0xC8,        // COM scan remap
    0xDA, 0x12,  // COM pins
    0x81, 0x7F,  // Contrast
    0xD9, 0x22,  // Pre-charge
    0xDB, 0x20,  // VCOMH
    0xA4,        // Resume RAM
    0xA6,        // Normal display
    0xAF         // Display ON
};

constexpr uint8_t Sh1106PageCmdBase = 0xB0U;
constexpr uint8_t Sh1106UpperColCmdBase = 0x10U;
constexpr uint8_t Sh1106LowerColCmdBase = 0x00U;
constexpr uint8_t Sh1106MaxColumn = 131U;

static size_t expectedWindowLen(const uint8_t col0, const uint8_t col1, const uint8_t page0,
                                const uint8_t page1) {
    // +1 because the ranges are inclusive
    const size_t w = static_cast<size_t>(col1 - col0 + 1UL);
    const size_t p = static_cast<size_t>(page1 - page0 + 1UL);
    return (w * p);
}

static bool toSh1106Column(const uint8_t logicalColumn, uint8_t& outPhysicalColumn) {
    const uint16_t physical = static_cast<uint16_t>(logicalColumn) + common::OLED_COLUMN_OFFSET;
    if (physical > Sh1106MaxColumn) {
        return false;
    }

    outPhysicalColumn = static_cast<uint8_t>(physical);
    return true;
}

}  // namespace

Display::Display(II2cBus& i2cBus) : mI2cBus(i2cBus), mI2cAddr(common::OLED_I2C_ADDR) {
    ESP_LOGI(Tag, "Creating Display");
}

bool Display::init() {
    ESP_LOGI(Tag, "Initializing Display at I2C address 0x%02X", mI2cAddr);

    if (!writeCommand(InitCmd.data(), InitCmd.size())) {
        ESP_LOGE(Tag, "Failed to write init command");
        return false;
    }

    mReady = true;
    ESP_LOGI(Tag, "Display ready");
    return true;
}

bool Display::showFramebuffer(const uint8_t* framebuffer, const size_t len) {
    // Full screen: cols 0..127, pages 0..7
    return showWindow(0U, static_cast<uint8_t>(DisplayWidth - 1U), 0U,
                      static_cast<uint8_t>(DisplayPages - 1U), framebuffer, len);
}

bool Display::showWindow(const uint8_t col0, const uint8_t col1, const uint8_t page0,
                         const uint8_t page1, const uint8_t* data, const size_t len) {
    if (!mReady || !data || len == 0UL) {
        return false;
    }
    if (col0 > col1 || page0 > page1) {
        ESP_LOGW(Tag, "Invalid window (col %u..%u, page %u..%u)", col0, col1, page0, page1);
        return false;
    }
    if (col1 >= DisplayWidth || page1 >= DisplayPages) {
        ESP_LOGW(Tag, "Window out of bounds (col %u..%u, page %u..%u)", col0, col1, page0, page1);
        return false;
    }

    const size_t expLen = expectedWindowLen(col0, col1, page0, page1);
    if (len != expLen) {
        ESP_LOGW(Tag, "Window len mismatch: got=%u expected=%u (col %u..%u page %u..%u)",
                 static_cast<unsigned>(len), static_cast<unsigned>(expLen), col0, col1, page0,
                 page1);
        return false;
    }

    // TODO: improve adaptation to SH1106
    uint8_t physicalCol0 = 0U;
    if (!toSh1106Column(col0, physicalCol0)) {
        ESP_LOGW(Tag, "Invalid mapped SH1106 column for col0=%u", col0);
        return false;
    }
    uint8_t physicalCol1 = 0U;
    if (!toSh1106Column(col1, physicalCol1)) {
        ESP_LOGW(Tag, "Invalid mapped SH1106 column for col1=%u", col1);
        return false;
    }
    (void)physicalCol1;

    const size_t width = static_cast<size_t>(col1 - col0 + 1U);
    const uint8_t* pageData = data;
    for (uint8_t page = page0; page <= page1; ++page) {
        const uint8_t pageCmd[3] = {
            static_cast<uint8_t>(Sh1106PageCmdBase + page),
            static_cast<uint8_t>(Sh1106UpperColCmdBase | ((physicalCol0 >> 4U) & 0x0FU)),
            static_cast<uint8_t>(Sh1106LowerColCmdBase | (physicalCol0 & 0x0FU)),
        };

        if (!writeCommand(pageCmd, sizeof(pageCmd))) {
            ESP_LOGW(Tag, "Failed to set page/column start (page=%u)", page);
            return false;
        }
        if (!writeData(pageData, width)) {
            ESP_LOGW(Tag, "Failed to write page data (page=%u)", page);
            return false;
        }

        pageData += width;
    }

    return true;
}

bool Display::writeCommand(const uint8_t* cmd, const size_t len) {
    if (!cmd || len == 0) {
        return false;
    }

    std::array<uint8_t, 1 + ChunkBytes> buf{};
    buf[0] = ControlByteCommand;
    const uint8_t* p = cmd;

    size_t writtenBytes = 0;
    while (writtenBytes < len) {
        const size_t chunk = std::min(ChunkBytes, (len - writtenBytes));

        // +1 to skip control byte
        std::memcpy(buf.data() + 1, p, chunk);

        if (!mI2cBus.writeBytes(mI2cAddr, buf.data(), 1 + chunk, 5000)) {
            ESP_LOGE(Tag, "Failed to write command (chunk=%u)", static_cast<unsigned>(chunk));
            return false;
        }

        p += chunk;
        writtenBytes += chunk;
    }

    return true;
}

bool Display::writeData(const uint8_t* data, const size_t len) {
    if (!data || len == 0) {
        return false;
    }

    std::array<uint8_t, 1 + ChunkBytes> buf{};
    buf[0] = ControlByteData;

    const uint8_t* p = data;
    size_t writtenBytes = 0;
    while (writtenBytes < len) {
        const size_t chunk = std::min(ChunkBytes, (len - writtenBytes));
        std::memcpy(buf.data() + 1, p, chunk);

        if (!mI2cBus.writeBytes(mI2cAddr, buf.data(), 1 + chunk, 5000)) {
            ESP_LOGE(Tag, "Failed to write data (chunk=%u)", static_cast<unsigned>(chunk));
            return false;
        }

        p += chunk;
        writtenBytes += chunk;
    }

    return true;
}

}  // namespace adapters
