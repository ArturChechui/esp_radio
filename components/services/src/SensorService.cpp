#include "SensorService.hpp"

#include <esp_log.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "BoardConfig.hpp"
#include "Events.hpp"
#include "IAdcReader.hpp"
#include "IClock.hpp"
#include "IEventQueue.hpp"
#include "II2cBus.hpp"
#include "IPlayerService.hpp"
#include "IStopToken.hpp"
#include "ITaskRunner.hpp"

namespace services {
namespace {
constexpr const char* Tag = "SensorService";

constexpr uint16_t TaskStackWords = 6144;
constexpr uint16_t TaskPriority = 6U;
constexpr uint16_t TaskCore = 0U;

constexpr uint8_t Aht20Addr = 0x38;
constexpr uint8_t BusyBit = 0x80;
constexpr uint8_t CalibrationEnableBit = 0x08;

constexpr uint32_t SensorLoopSleepMs = 20U;
constexpr uint32_t TempHumidPeriodMs = 30000U;
constexpr uint32_t LightPeriodMs = 30000U;
constexpr uint32_t BatteryPeriodMs = 30000U;

constexpr uint8_t Bh1750OneTimeHighResMode = 0x20;
constexpr uint32_t Bh1750MeasureWaitMs = 180U;

// Tuned for MAX4466 + ESP32 ADC in noisy environments.
constexpr uint32_t MicTaskSleepMs = 5U;
constexpr size_t ClapBurstSamples = 96U;  // TODO: make it more usable. try 64
constexpr int32_t ClapMinEnergyRaw = 16;
constexpr int32_t ClapMinPeakDiffRaw = 220;
constexpr int32_t ClapNoiseMultiplierNum = 5;
constexpr int32_t ClapNoiseMultiplierDen = 2;
constexpr int32_t ClapMinP2PRaw = 460;
constexpr int32_t ClapP2PToEnergyRatio = 6;
constexpr uint8_t ClapMinSamplesAboveThreshold = 1U;
constexpr uint32_t ClapCooldownMs = 300U;
constexpr uint32_t ClapArmDelayAfterPlaybackMs = 700U;
constexpr uint32_t ClapDiagLogPeriodMs = 300U;
constexpr uint32_t ClapTraceLogPeriodMs = 1500U;
constexpr int32_t NoiseEmaAlphaDiv = 32;
}  // namespace

SensorService::SensorService(adapters::II2cBus& i2cBus, adapters::IAdcReader& adcReader,
                             common::IEventQueue& coreEventQueue, common::ITaskRunner& runner,
                             common::IClock& clock)
    : mI2cBus(i2cBus),
      mAdcReader(adcReader),
      mCoreEventQueue(coreEventQueue),
      mTaskRunner(runner),
      mClock(clock),
      mSensorTaskHandle(),
      mMicTaskHandle(),
      mIsAht20Inited(false),
      mTempHumidElapsedMs(TempHumidPeriodMs),
      mLightElapsedMs(0U),
      mBatteryElapsedMs(0U),
      mClapCooldownMs(0U),
      mMicBaselineMv(-1),
      mMicNoiseMv(0),
      mMicAboveThresholdCount(0U),
      mWasPlaybackActive(false),
      mPlaybackActive(false) {}

SensorService::~SensorService() {
    deinit();
}

bool SensorService::init() {
    if (!mAdcReader.setupChannel(common::MicAdcGpio)) {
        ESP_LOGE(Tag, "Failed to setup mic channel");
        return false;
    }

    if (!mAdcReader.setupChannel(common::BatteryAdcGpio)) {
        ESP_LOGE(Tag, "Failed to setup battery channel");
        return false;
    }

    mSensorTaskHandle = mTaskRunner.start(
        common::TaskParams{.name = "SensorTask", .priority = TaskPriority, .core = TaskCore},
        TaskStackWords, &SensorService::readStepFn, this);
    if (!mSensorTaskHandle.isValid()) {
        ESP_LOGE(Tag, "Failed to create SensorTask");
        return false;
    }

    mMicTaskHandle = mTaskRunner.start(
        common::TaskParams{.name = "MicTask", .priority = TaskPriority, .core = TaskCore},
        TaskStackWords, &SensorService::listenStepFn, this);
    if (!mMicTaskHandle.isValid()) {
        ESP_LOGE(Tag, "Failed to create MicTask");

        (void)mTaskRunner.stop(mSensorTaskHandle, 7000);
        mSensorTaskHandle.reset();

        return false;
    }
    return true;
}

void SensorService::deinit() {
    if (mMicTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mMicTaskHandle, 7000);
        mMicTaskHandle.reset();
    }

    if (mSensorTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mSensorTaskHandle, 7000);
        mSensorTaskHandle.reset();
    }
}

void SensorService::setPlaybackActive(const bool active) {
    if (active) {
        if (!mMicTaskHandle.isValid()) {
            return;  // Already stopped
        }

        ESP_LOGI(Tag, "Playback started: Stopping clap detection...");
        (void)mTaskRunner.stop(mMicTaskHandle, 2000);
        mMicTaskHandle.reset();
        mMicNoiseMv = 0;
        mMicAboveThresholdCount = 0U;
    } else {
        if (mMicTaskHandle.isValid()) {
            return;  // Already listening
        }

        ESP_LOGI(Tag, "Playback stopped: Starting clap detection in 1 second...");
        mClock.sleepMs(1000);
        mMicTaskHandle = mTaskRunner.start(
            common::TaskParams{.name = "MicTask", .priority = TaskPriority, .core = TaskCore},
            TaskStackWords, &SensorService::listenStepFn, this);
        if (!mMicTaskHandle.isValid()) {
            ESP_LOGE(Tag, "Failed to create MicTask");
        }
    }
}

common::StepResult SensorService::listenStepFn(void* arg, common::IStopToken& token) {
    auto* self = static_cast<SensorService*>(arg);
    if (!self) {
        return {.action = common::StepAction::Error};
    }

    return self->listenStep(token);
}

common::StepResult SensorService::listenStep(common::IStopToken& token) {
    if (token.stopRequested()) {
        return {.action = common::StepAction::Done};
    }

    int rawBuffer[ClapBurstSamples];
    if (!mAdcReader.readRawBurst(common::MicAdcGpio, rawBuffer, ClapBurstSamples)) {
        return {.action = common::StepAction::Sleep, .sleepMs = MicTaskSleepMs};
    }

    const auto feat = analyzeBurst(rawBuffer, ClapBurstSamples);

    // Calculate dynamic thresholds
    const int32_t energyTh =
        std::max(ClapMinEnergyRaw, (mMicNoiseMv * ClapNoiseMultiplierNum) / ClapNoiseMultiplierDen);
    const int32_t p2pTh = std::max(ClapMinP2PRaw, energyTh * ClapP2PToEnergyRatio);

    const bool isClap =
        (feat.energy >= energyTh) && (feat.peakDiff >= ClapMinPeakDiffRaw) && (feat.p2p >= p2pTh);

    logMicDiagnostics(feat, energyTh, p2pTh, isClap, rawBuffer);

    if (isClap) {
        ESP_LOGI(Tag, "Clap! Stats: energy=%d/%d, p2p=%d/%d, peak=%d",
                 static_cast<int>(feat.energy), static_cast<int>(energyTh),
                 static_cast<int>(feat.p2p), static_cast<int>(p2pTh),
                 static_cast<int>(feat.peakDiff));
        (void)mCoreEventQueue.post(common::ButtonPressedEvent{common::Button::PlayStop});

        return {.action = common::StepAction::Done};
    }

    if (mMicNoiseMv <= 0) {
        mMicNoiseMv = feat.energy;
    } else {
        mMicNoiseMv = (mMicNoiseMv * (NoiseEmaAlphaDiv - 1) + feat.energy) / NoiseEmaAlphaDiv;
    }

    return {.action = common::StepAction::Sleep, .sleepMs = MicTaskSleepMs};
}

common::MicFeatures SensorService::analyzeBurst(const int* buffer, const size_t len) {
    if (len == 0) {
        return {};
    }

    int32_t minRaw = buffer[0], maxRaw = buffer[0], prevRaw = buffer[0];
    int32_t absDiffSum = 0, peakDiff = 0;

    for (size_t i = 1; i < len; ++i) {
        int32_t val = buffer[i];
        minRaw = std::min(minRaw, val);
        maxRaw = std::max(maxRaw, val);

        int32_t diff = std::abs(val - prevRaw);
        absDiffSum += diff;
        peakDiff = std::max(peakDiff, diff);
        prevRaw = val;
    }

    return {.energy = (absDiffSum / static_cast<int32_t>(std::max<size_t>(1, len - 1))),
            .p2p = (maxRaw - minRaw),
            .peakDiff = peakDiff,
            .minRaw = minRaw,
            .maxRaw = maxRaw};
}

void SensorService::logMicDiagnostics(const common::MicFeatures& f, const int32_t energyTh,
                                      const int32_t p2pTh, const bool candidate, const int* raw) {
    const uint64_t now = mClock.nowMs();

    // 1. Periodic Stats Log
    static uint64_t lastDiagMs = 0;
    if (now - lastDiagMs >= ClapDiagLogPeriodMs) {
        lastDiagMs = now;
        ESP_LOGI(Tag, "Stats: energy=%d/%d, p2p=%d/%d, peak=%d", static_cast<int>(f.energy),
                 static_cast<int>(energyTh), static_cast<int>(f.p2p), static_cast<int>(p2pTh),
                 static_cast<int>(f.peakDiff));
    }

    // 2. Near-Miss Trace Log
    static uint64_t lastTraceMs = 0;
    bool nearMiss = !candidate && (f.energy * 5 >= energyTh * 4 || f.p2p * 5 >= p2pTh * 4);

    if (nearMiss && (now - lastTraceMs >= ClapTraceLogPeriodMs)) {
        lastTraceMs = now;
        ESP_LOGI(Tag, "Trace: %d,%d,%d,%d,%d,%d,%d,%d", raw[0], raw[4], raw[8], raw[12], raw[16],
                 raw[20], raw[24], raw[28]);
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

    // Temp
    int8_t tempC = 0;
    uint8_t hum = 0;
    if (readAht20(token, tempC, hum)) {
        ESP_LOGI(Tag, "Sensor values: Temp=%dC, Hum=%u%%", tempC, hum);
        mCoreEventQueue.post(common::TempHumidUpdateEvent{.temperature = tempC, .humidity = hum});
    }

    // Light
    uint16_t lux = 0;
    if (readBh1750(token, lux)) {
        ESP_LOGI(Tag, "Light value: %u lux", static_cast<unsigned>(lux));
        mCoreEventQueue.post(common::LightLevelUpdateEvent{.lux = lux});
    }
    mLightElapsedMs = 0U;

    // Battery
    uint16_t batteryMv = 0;
    uint8_t batteryPct = 0;
    if (readBattery(batteryMv, batteryPct)) {
        ESP_LOGI(Tag, "Battery: %u mV (%u%%)", static_cast<unsigned>(batteryMv),
                 static_cast<unsigned>(batteryPct));
        mCoreEventQueue.post(common::BatteryLevelUpdateEvent{
            .millivolts = batteryMv,
            .percent = batteryPct,
        });
    }
    mBatteryElapsedMs = 0U;

    return {.action = common::StepAction::Sleep, .sleepMs = 30000};
}

bool SensorService::readAht20(common::IStopToken& token, int8_t& temperatureC,
                              uint8_t& humidityPct) {
    if (!mIsAht20Inited && !initAht20(token)) {
        return false;
    }

    const std::array<uint8_t, 3> triggerCmd = {0xAC, 0x33, 0x00};
    if (!mI2cBus.writeBytes(Aht20Addr, triggerCmd.data(), triggerCmd.size(), 100)) {
        ESP_LOGE(Tag, "Failed to trigger AHT20 measurement");
        return false;
    }

    if (token.sleepMs(80)) {
        return false;
    }

    std::array<uint8_t, 6> data;
    if (!mI2cBus.readBytes(Aht20Addr, data.data(), data.size(), 100)) {
        ESP_LOGE(Tag, "Failed to read from AHT20");
        return false;
    }

    const uint8_t status = data[0];
    if (!(status & CalibrationEnableBit)) {
        ESP_LOGE(Tag, "AHT20 not calibrated, re-initializing. Status byte: 0x%02X", status);
        mIsAht20Inited = false;
        return false;
    }

    return aht20Parse(data, temperatureC, humidityPct);
}

bool SensorService::readBh1750(common::IStopToken& token, uint16_t& lux) {
    const std::array<uint8_t, 1> modeCmd = {Bh1750OneTimeHighResMode};
    if (!mI2cBus.writeBytes(common::Bh1750I2cAddr, modeCmd.data(), modeCmd.size(), 100)) {
        ESP_LOGE(Tag, "Failed to trigger BH1750 measurement");
        return false;
    }

    if (token.sleepMs(Bh1750MeasureWaitMs)) {
        return false;
    }

    std::array<uint8_t, 2> data{};
    if (!mI2cBus.readBytes(common::Bh1750I2cAddr, data.data(), data.size(), 100)) {
        ESP_LOGE(Tag, "Failed to read from BH1750");
        return false;
    }

    const uint16_t raw = static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8U) | data[1]);
    lux = static_cast<uint16_t>((static_cast<uint32_t>(raw) * 10U + 6U) / 12U);

    return true;
}

bool SensorService::readBattery(uint16_t& batteryMv, uint8_t& batteryPct) {
    static constexpr size_t BurstSize = 5;
    int rawBuffer[BurstSize];
    if (!mAdcReader.readRawBurst(common::BatteryAdcGpio, rawBuffer, BurstSize)) {
        ESP_LOGE(Tag, "Failed to read battery ADC");
        return false;
    }

    uint32_t sumMv = 0U;
    for (size_t i = 0; i < BurstSize; ++i) {
        const int32_t clampedRaw =
            std::clamp(rawBuffer[i], 0, static_cast<int>(adapters::IAdcReader::MaxAdcRaw));
        const uint32_t sampleMv =
            (static_cast<uint32_t>(clampedRaw) * adapters::IAdcReader::MaxAdcMv) /
            adapters::IAdcReader::MaxAdcRaw;
        sumMv += sampleMv;
    }

    const uint32_t avgAdcMv = sumMv / BurstSize;
    const uint64_t numerator =
        static_cast<uint64_t>(avgAdcMv) * (common::BatterySenseR1Ohm + common::BatterySenseR2Ohm);
    batteryMv = static_cast<uint16_t>(numerator / common::BatterySenseR2Ohm);
    batteryPct = batteryPercentFromMillivolts(batteryMv);

    return true;
}

uint8_t SensorService::batteryPercentFromMillivolts(const uint16_t batteryMv) {
    constexpr uint16_t EmptyMv = 3300U;
    constexpr uint16_t FullMv = 4200U;

    if (batteryMv <= EmptyMv) {
        return 0U;
    }
    if (batteryMv >= FullMv) {
        return 100U;
    }

    return static_cast<uint8_t>(((batteryMv - EmptyMv) * 100U) / (FullMv - EmptyMv));
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
        return false;
    }

    const std::array<uint8_t, 3> initCmd = {0xBE, 0x08, 0x00};
    if (!mI2cBus.writeBytes(Aht20Addr, initCmd.data(), initCmd.size(), 100)) {
        ESP_LOGE(Tag, "Failed to send initialization command to AHT20");
        return false;
    }

    if (token.sleepMs(10)) {
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

    const uint32_t humRaw = (static_cast<uint32_t>(data[1]) << 12) |
                            (static_cast<uint32_t>(data[2]) << 4) |
                            (static_cast<uint32_t>(data[3]) >> 4);

    const uint32_t tempRaw = ((static_cast<uint32_t>(data[3]) & 0x0F) << 16) |
                             (static_cast<uint32_t>(data[4]) << 8) | static_cast<uint32_t>(data[5]);

    constexpr double denom = 1048576.0;
    const double humidity = (static_cast<double>(humRaw) * 100.0) / denom;
    const double temperature = (static_cast<double>(tempRaw) * 200.0) / denom - 50.0;

    humidityPct = static_cast<uint8_t>(std::round(std::clamp(humidity, 0.0, 99.0)));
    temperatureC = static_cast<int8_t>(std::round(std::clamp(temperature, -50.0, 99.0)));

    return true;
}

}  // namespace services
