#pragma once

#include <array>
#include <cstdint>

#include "ISensorService.hpp"
#include "Types.hpp"

namespace adapters {
class II2cBus;
class IAdcReader;
}  // namespace adapters

namespace common {
class IEventQueue;
class ITaskRunner;
class IClock;
}  // namespace common

namespace services {
class SensorService : public ISensorService {
   public:
    SensorService(adapters::II2cBus& i2cBus, adapters::IAdcReader& adcReader,
                  common::IEventQueue& coreEventQueue, common::ITaskRunner& runner,
                  common::IClock& clock);
    ~SensorService() override;

    bool init() override;
    void deinit() override;
    void setPlaybackActive(const bool active) override;

   private:
    static common::StepResult readStepFn(void* arg, common::IStopToken& token);
    common::StepResult readStep(common::IStopToken& token);
    static common::StepResult listenStepFn(void* arg, common::IStopToken& token);
    common::StepResult listenStep(common::IStopToken& token);

    common::MicFeatures analyzeBurst(const int* buffer, const size_t len);
    void logMicDiagnostics(const common::MicFeatures& f, const int32_t energyTh,
                           const int32_t p2pTh, const bool candidate, const int* raw);

    bool readAht20(common::IStopToken& token, int8_t& temperatureC, uint8_t& humidityPct);
    bool readBh1750(common::IStopToken& token, uint16_t& lux);
    bool readBattery(uint16_t& batteryMv, uint8_t& batteryPct);
    static uint8_t batteryPercentFromMillivolts(uint16_t batteryMv);
    bool aht20Parse(const std::array<uint8_t, 6>& data, int8_t& temperatureC, uint8_t& humidityPct);
    bool initAht20(common::IStopToken& token);

    adapters::II2cBus& mI2cBus;
    adapters::IAdcReader& mAdcReader;
    common::IEventQueue& mCoreEventQueue;
    common::ITaskRunner& mTaskRunner;
    common::IClock& mClock;
    common::TaskHandle mSensorTaskHandle;
    common::TaskHandle mMicTaskHandle;

    // TODO: clean unused members and constants
    bool mIsAht20Inited;
    uint32_t mTempHumidElapsedMs;
    uint32_t mLightElapsedMs;
    uint32_t mBatteryElapsedMs;
    uint32_t mClapCooldownMs;
    int32_t mMicBaselineMv;
    int32_t mMicNoiseMv;
    uint8_t mMicAboveThresholdCount;
    bool mWasPlaybackActive;
    bool mPlaybackActive;
};

}  // namespace services
