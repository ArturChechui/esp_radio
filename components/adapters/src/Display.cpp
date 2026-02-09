#include "Display.hpp"

#include <array>
#include <cstring>

#include "BoardConfig.hpp"
#include "II2cBus.hpp"

// IDF
#include <esp_log.h>

namespace adapters {
namespace {
constexpr const char *Tag = "Display";
constexpr uint8_t ControllByteCommand = 0x00;
constexpr uint8_t ControllByteData = 0x40;
constexpr std::array<uint8_t, 3> PageCmd = {0x22, 0x00, 0x07};  // Set page address 0-7
constexpr std::array<uint8_t, 3> ColCmd = {0x21, 0x00, 0x7F};   // Set column address 0-127
constexpr std::array<uint8_t, 25> InitCmd = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D,
                                             0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
                                             0xD9, 0xF1, 0xDB, 0x20, 0xA4, 0xA6, 0xAF};
}  // namespace

Display::Display(II2cBus &i2cBus)
    : mI2cBus(i2cBus), mI2cAddr(common::OLED_I2C_ADDR), mReady(false) {
    ESP_LOGI(Tag, "Creating Display");
}

Display::~Display() {}

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

bool Display::showFramebuffer(const uint8_t *framebuffer, const size_t &len) {
    if (!mReady) {
        ESP_LOGW(Tag, "Display not ready");
        return false;
    }

    bool ret = writeCommand(PageCmd.data(), PageCmd.size());
    if (!ret) {
        ESP_LOGE(Tag, "Failed to write page command");
        return false;
    }

    ret = writeCommand(ColCmd.data(), ColCmd.size());
    if (!ret) {
        ESP_LOGE(Tag, "Failed to write column command");
        return false;
    }

    ret = writeData(framebuffer, len);
    if (!ret) {
        ESP_LOGE(Tag, "Failed to write framebuffer data");
        return false;
    }

    return true;
}

bool Display::writeCommand(const uint8_t *cmd, const uint16_t &len) {
    std::vector<uint8_t> buf(len + 1U);
    buf[0] = ControllByteCommand;
    std::memcpy(&buf[1], cmd, len);

    const bool ret = mI2cBus.writeBytes(mI2cAddr, buf.data(), buf.size(), 1000U);
    if (!ret) {
        ESP_LOGE(Tag, "Failed to write command to display");
    }

    return ret;
}

bool Display::writeData(const uint8_t *data, const uint16_t &len) {
    std::vector<uint8_t> buf(len + 1U);
    buf[0] = ControllByteData;
    std::memcpy(&buf[1], data, len);

    const bool ret = mI2cBus.writeBytes(mI2cAddr, buf.data(), buf.size(), 1000U);
    if (!ret) {
        ESP_LOGE(Tag, "Failed to write data to display");
    }

    return ret;
}

}  // namespace adapters
