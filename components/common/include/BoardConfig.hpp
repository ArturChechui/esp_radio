/**
 * @file BoardConfig.hpp
 * @brief Centralized hardware pin and peripheral configuration.
 *
 * This file defines the GPIO assignments and hardware constants for the
 * ESP32-based internet radio, including display, audio, and sensor pins.
 */

#pragma once

#include <array>
#include <cstdint>

/**
 * @namespace common
 * @brief Contains shared system-wide constants, types, and configurations.
 */
namespace common {

// I2C Configuration
/** @brief GPIO pin used for the I2C Serial Clock (SCL). */
static constexpr int I2C_SCL_GPIO = 1;  // GP1
/** @brief GPIO pin used for the I2C Serial Data (SDA). */
static constexpr int I2C_SDA_GPIO = 2;  // GP2
/** @brief I2C bus frequency in Hertz. */
static constexpr uint32_t I2C_FREQ_HZ = 100000;  // TODO: test with 400000 later
/** @brief I2C slave address for the OLED display. */
static constexpr uint8_t OLED_I2C_ADDR = 0x3C;
/** @brief I2C slave address for the BH1750 ambient light sensor. */
static constexpr uint8_t Bh1750I2cAddr = 0x23;
// TODO: SH1106 usually needs a +2 column shift. Set to 0 if image appears horizontally shifted.
/** @brief Horizontal column offset required for the specific OLED controller. */
static constexpr uint8_t OLED_COLUMN_OFFSET = 2;

// I2S Configuration (audio output)
/** @brief GPIO pin for the I2S Bit Clock (BCLK). */
static constexpr uint8_t I2S_BCLK_GPIO = 6;
/** @brief GPIO pin for the I2S Left/Right Clock (LRCK / Word Select). */
static constexpr uint8_t I2S_LRCK_GPIO = 5;
/** @brief GPIO pin for the I2S Data Out (DOUT). */
static constexpr uint8_t I2S_DOUT_GPIO = 7;
/** @brief Target audio sample rate in Hertz. */
static constexpr uint32_t I2S_SAMPLE_RATE = 44100;

// Buttons
/** @brief GPIO pin for the "Next Station" button. */
static constexpr uint64_t ButtonNextGpio = 4;
/** @brief GPIO pin for the "Previous Station" button. */
static constexpr uint64_t ButtonPrevGpio = 10;
/** @brief GPIO pin for the "Play/Stop" toggle button. */
static constexpr uint64_t ButtonPlayStopGpio = 9;
/** @brief Array containing all managed button GPIOs for batch initialization. */
static constexpr std::array<uint64_t, 3> ButtonGpios = {
    ButtonPlayStopGpio,
    ButtonNextGpio,
    ButtonPrevGpio,
};
/** @brief GPIO pin for the first phase signal (S1) of the rotary encoder. */
static constexpr uint64_t EncS1Gpio = 13;
/** @brief GPIO pin for the second phase signal (S2) of the rotary encoder. */
static constexpr uint64_t EncS2Gpio = 12;

// Analog inputs
/** @brief GPIO pin assigned to the ADC channel for microphone input. */
static constexpr uint64_t MicAdcGpio = 8;
/** @brief GPIO pin assigned to the ADC channel for battery voltage sensing. */
static constexpr uint64_t BatteryAdcGpio = 3;

// Battery sensing divider: BAT+ -> R1 -> ADC -> R2 -> GND
/** @brief Resistance of the high-side resistor (R1) in the battery voltage divider. */
static constexpr uint32_t BatterySenseR1Ohm = 100000U;
/** @brief Resistance of the low-side resistor (R2) in the battery voltage divider. */
static constexpr uint32_t BatterySenseR2Ohm = 100000U;

}  // namespace common
