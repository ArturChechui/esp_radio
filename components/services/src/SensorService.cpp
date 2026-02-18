#include "SensorService.hpp"

#include <esp_log.h>

#include <algorithm>
#include <cmath>

#include "Events.hpp"
#include "IEventQueue.hpp"
#include "II2cBus.hpp"
#include "IStopToken.hpp"
#include "ITaskRunner.hpp"

namespace services {
namespace {
constexpr const char* Tag = "SensorService";

constexpr uint16_t TaskStackWords = 6144;
constexpr uint16_t TaskPriority = 6U;
constexpr uint16_t TaskCore = 0U;

constexpr uint8_t Aht20Addr = 0x38;
constexpr uint8_t Bmp280Addr = 0x76;  // or 0x77
constexpr uint8_t BusyBit = 0x80;
constexpr uint8_t CalibrationEnableBit = 0x08;
}  // namespace

SensorService::SensorService(adapters::II2cBus& i2cBus, common::IEventQueue& coreEventQueue,
                             common::ITaskRunner& runner)
    : mI2cBus(i2cBus),
      mCoreEventQueue(coreEventQueue),
      mTaskRunner(runner),
      mTaskHandle(),
      mIsAht20Inited(false) {}

SensorService::~SensorService() {
    deinit();
}

bool SensorService::init() {
    mTaskHandle = mTaskRunner.start(
        common::TaskParams{.name = "SensorTask", .priority = TaskPriority, .core = TaskCore},
        TaskStackWords, &SensorService::readStepFn, this);
    if (!mTaskHandle.isValid()) {
        ESP_LOGE(Tag, "Failed to create SensorTask");
        return false;
    }
    return true;
}

void SensorService::deinit() {
    if (mTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mTaskHandle, 7000);
        mTaskHandle.reset();
    }
}

common::StepResult SensorService::readStepFn(void* arg, common::IStopToken& token) {
    auto* self = static_cast<SensorService*>(arg);
    if (!self) {
        return {.action = common::StepAction::Error};
    }

    return self->readStep(token);
}

common::StepResult SensorService::readStep(common::IStopToken& token) {
    if (token.stopRequested()) {
        return {.action = common::StepAction::Done};
    }

    if (!mIsAht20Inited && !initAht20(token)) {
        return {.action = common::StepAction::Sleep, .sleepMs = 5000U};
    }

    const std::array<uint8_t, 3> triggerCmd = {0xAC, 0x33, 0x00};
    if (!mI2cBus.writeBytes(Aht20Addr, triggerCmd.data(), triggerCmd.size(), 100)) {
        ESP_LOGE(Tag, "Failed to trigger AHT20 measurement");
        return {.action = common::StepAction::Sleep, .sleepMs = 30000U};
    }

    if (token.sleepMs(80)) {
        return {.action = common::StepAction::Done};
    }

    std::array<uint8_t, 6> data;
    if (!mI2cBus.readBytes(Aht20Addr, data.data(), data.size(), 100)) {
        ESP_LOGE(Tag, "Failed to read from AHT20");
        return {.action = common::StepAction::Sleep, .sleepMs = 30000U};
    }

    const uint8_t status = data[0];
    if (!(status & CalibrationEnableBit)) {
        ESP_LOGE(Tag, "AHT20 not calibrated, re-initializing. Status byte: 0x%02X", status);
        mIsAht20Inited = false;
        return {.action = common::StepAction::Sleep, .sleepMs = 1000U};
    }

    int8_t tempC = 0;
    uint8_t hum = 0;
    if (!aht20Parse(data, tempC, hum)) {
        return {.action = common::StepAction::Sleep, .sleepMs = 30000U};
    }

    ESP_LOGI(Tag, "Sensor values: Temp=%dC, Hum=%u%%", tempC, hum);
    mCoreEventQueue.post(common::TempHumidUpdateEvent{.temperature = tempC, .humidity = hum});

    return {.action = common::StepAction::Sleep, .sleepMs = 30000U};
}

bool SensorService::initAht20(common::IStopToken& token) {
    if (mIsAht20Inited) {
        ESP_LOGW(Tag, "AHT20 already inited");
        return true;
    }

    ESP_LOGI(Tag, "Initializing AHT20 sensor...");

    const std::array<uint8_t, 1> resetCmd = {0xBA};
    if (!mI2cBus.writeBytes(Aht20Addr, resetCmd.data(), resetCmd.size(), 100)) {
        ESP_LOGE(Tag, "Failed to send soft reset to AHT20");
        return false;
    }

    if (token.sleepMs(20)) {
        // sleep was interrupted
        return false;
    }

    const std::array<uint8_t, 3> initCmd = {0xBE, 0x08, 0x00};
    if (!mI2cBus.writeBytes(Aht20Addr, initCmd.data(), initCmd.size(), 100)) {
        ESP_LOGE(Tag, "Failed to send initialization command to AHT20");
        return false;
    }

    if (token.sleepMs(10)) {
        // sleep was interrupted
        return false;
    }

    mIsAht20Inited = true;
    ESP_LOGI(Tag, "AHT20 initialized successfully.");
    return true;
}

bool SensorService::aht20Parse(const std::array<uint8_t, 6>& data, int8_t& temperatureC,
                               uint8_t& humidityPct) {
    if (data[0] & BusyBit) {
        ESP_LOGW(Tag, "AHT20 busy");
        return false;
    }

    const uint32_t hum_raw = (static_cast<uint32_t>(data[1]) << 12) |
                             (static_cast<uint32_t>(data[2]) << 4) |
                             (static_cast<uint32_t>(data[3]) >> 4);

    const uint32_t temp_raw = ((static_cast<uint32_t>(data[3]) & 0x0F) << 16) |
                              (static_cast<uint32_t>(data[4]) << 8) |
                              static_cast<uint32_t>(data[5]);

    constexpr double denom = 1048576.0;
    const double humidity = (static_cast<double>(hum_raw) * 100.0) / denom;
    const double temperature = (static_cast<double>(temp_raw) * 200.0) / denom - 50.0;

    humidityPct = static_cast<uint8_t>(std::round(std::clamp(humidity, 0.0, 99.0)));
    temperatureC = static_cast<int8_t>(std::round(std::clamp(temperature, -50.0, 99.0)));

    return true;
}

}  // namespace services
