#pragma once

#include <array>
#include <cstdint>

namespace common {

// I2C Configuration
static constexpr int I2C_SCL_GPIO = 1;           // GP1
static constexpr int I2C_SDA_GPIO = 2;           // GP2
static constexpr uint32_t I2C_FREQ_HZ = 100000;  // TODO: test with 400000 later
static constexpr uint8_t OLED_I2C_ADDR = 0x3C;

// I2S Configuration (audio output)
static constexpr uint8_t I2S_BCLK_GPIO = 6;
static constexpr uint8_t I2S_LRCK_GPIO = 5;
static constexpr uint8_t I2S_DOUT_GPIO = 7;
static constexpr uint32_t I2S_SAMPLE_RATE = 44100;

// Buttons
static constexpr uint64_t ButtonNextGpio = 8;
static constexpr uint64_t ButtonPrevGpio = 9;
static constexpr uint64_t ButtonPlayStopGpio = 10;
static constexpr std::array<uint64_t, 3> ButtonGpios = {
    ButtonPlayStopGpio,
    ButtonNextGpio,
    ButtonPrevGpio,
};
static constexpr uint64_t EncS1Gpio = 13;
static constexpr uint64_t EncS2Gpio = 12;
}  // namespace common
