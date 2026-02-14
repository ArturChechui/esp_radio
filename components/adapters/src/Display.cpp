#include "Display.hpp"

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

// SSD1306 init sequence (128x64). Addressing mode: Horizontal (0x20, 0x00).
constexpr std::array<uint8_t, 25> InitCmd = {
    0xAE,        // Display OFF
    0xD5, 0x80,  // Clock
    0xA8, 0x3F,  // MUX
    0xD3, 0x00,  // Offset
    0x40,        // Start line
    0x8D, 0x14,  // Charge pump enable
    0x20, 0x00,  // Horizontal addressing
    0xA1,        // Segment remap
    0xC8,        // COM scan remap
    0xDA, 0x12,  // COM pins
    0x81, 0x7F,  // Contrast
    0xD9, 0xF1,  // Pre-charge
    0xDB, 0x20,  // VCOMH
    0xA4,        // Resume RAM
    0xA6,        // Normal display
    0xAF         // Display ON
};

static size_t expectedWindowLen(const uint8_t col0, const uint8_t col1, const uint8_t page0,
                                const uint8_t page1) {
    // +1 because the ranges are inclusive
    const size_t w = static_cast<size_t>(col1 - col0 + 1UL);
    const size_t p = static_cast<size_t>(page1 - page0 + 1UL);
    return (w * p);
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
    return showWindow(0U, 127U, 0U, 7U, framebuffer, len);
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

    const size_t expLen = expectedWindowLen(col0, col1, page0, page1);
    if (len != expLen) {
        ESP_LOGW(Tag, "Window len mismatch: got=%u expected=%u (col %u..%u page %u..%u)",
                 static_cast<unsigned>(len), static_cast<unsigned>(expLen), col0, col1, page0,
                 page1);
        return false;
    }

    const uint8_t colCmd[3] = {0x21, col0, col1};
    // Set page address range
    const uint8_t pageCmd[3] = {0x22, page0, page1};

    if (!writeCommand(colCmd, sizeof(colCmd))) {
        ESP_LOGW(Tag, "Failed to set column range");
        return false;
    }
    if (!writeCommand(pageCmd, sizeof(pageCmd))) {
        ESP_LOGW(Tag, "Failed to set page range");
        return false;
    }

    return writeData(data, len);
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
