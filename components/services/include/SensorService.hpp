/**
 * @file SensorService.hpp
 * @brief Implementation of the ISensorService interface for environmental and audio sensing.
 *
 * This file contains the SensorService class, which manages periodic sensor polling
 * (AHT20, BH1750, Battery) and performs burst analysis on audio data from an ADC/Microphone.
 */

#pragma once

#include <array>
#include <cstdint>

#include "ISensorService.hpp"
#include "Types.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer interfaces.
 */
namespace adapters {
class II2cBus;
class IAdcReader;
class IPersistentStorage;
}  // namespace adapters

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {
class IEventQueue;
class ITaskRunner;
class IClock;
}  // namespace common

/**
 * @namespace services
 * @brief Contains business logic service implementations.
 */
namespace services {

/**
 * @class SensorService
 * @brief Concrete implementation of environmental sensing and audio monitoring logic.
 *
 * SensorService runs two main background tasks:
 * 1. A sensor task that periodically reads I2C sensors (Temperature, Humidity, Lux)
 * and the internal battery voltage.
 * 2. A microphone task that analyzes audio bursts to detect significant acoustic
 * energy or patterns, dispatching events to the core when thresholds are met.
 */
class SensorService : public ISensorService {
   public:
    /**
     * @brief Constructs a SensorService with its required hardware and system dependencies.
     * @param i2cBus Reference to the I2C bus for environmental sensors.
     * @param adcReader Reference to the ADC adapter for microphone and battery readings.
     * @param coreEventQueue Queue for dispatching detected environmental or audio events.
     * @param runner The task runner used to manage background sensing threads.
     * @param clock Reference to the system clock for timing and polling intervals.
     * @param persistentStorage Storage adapter for loading/saving sensor-related settings.
     */
    SensorService(adapters::II2cBus& i2cBus, adapters::IAdcReader& adcReader,
                  common::IEventQueue& coreEventQueue, common::ITaskRunner& runner,
                  common::IClock& clock, adapters::IPersistentStorage& persistentStorage);

    /** @brief Default virtual destructor. */
    ~SensorService() override;

    /**
     * @brief Initializes sensor hardware and starts background sensing tasks.
     * @return true if all sensors and tasks were successfully initialized.
     */
    bool init() override;

    /**
     * @brief Stops background tasks and releases sensor-related resources.
     */
    void deinit() override;

    /**
     * @brief Starts or stops the clap detection task.
     * @param shouldStart True to request activation. Ignored if the feature is disabled.
     */
    void startClapDetection(const bool shouldStart) override;

    /**
     * @brief Toggles the master enable/disable state for the clap detection feature.
     * @note Changes are persisted to storage and will override active detection requests.
     * @return True if the feature is now enabled, false if it is now disabled.
     */
    bool toggleClapFeature() override;

   private:
    /**
     * @brief Static entry point for the environmental sensor polling task.
     */
    static common::StepResult readStepFn(void* arg, common::IStopToken& token);

    /**
     * @brief Logic for the environmental sensor task (AHT20, BH1750, Battery).
     */
    common::StepResult readStep(common::IStopToken& token);

    /**
     * @brief Static entry point for the microphone analysis task.
     */
    static common::StepResult listenStepFn(void* arg, common::IStopToken& token);

    /**
     * @brief Logic for the microphone task, analyzing audio chunks for activity.
     */
    common::StepResult listenStep(common::IStopToken& token);

    /**
     * @brief Analyzes a raw audio burst to extract features like peak-to-peak and energy.
     * @param buffer Pointer to raw ADC samples.
     * @param len Number of samples in the buffer.
     * @return common::MicFeatures Calculated audio characteristics.
     */
    common::MicFeatures analyzeBurst(const int* buffer, const size_t len);

    /**
     * @brief Logs detailed diagnostic information for audio analysis.
     */
    void logMicDiagnostics(const common::MicFeatures& f, const int32_t energyTh,
                           const int32_t p2pTh, const bool candidate, const int* raw);

    /** @brief Reads temperature and humidity from the AHT20 sensor. */
    bool readAht20(common::IStopToken& token, int8_t& temperatureC, uint8_t& humidityPct);

    /** @brief Reads ambient light levels from the BH1750 sensor. */
    bool readBh1750(common::IStopToken& token, uint16_t& lux);

    /** @brief Reads the system battery voltage and calculates percentage. */
    bool readBattery(uint16_t& batteryMv, uint8_t& batteryPct);

    /** @brief Helper to map millivolts to a 0-100% battery level. */
    static uint8_t batteryPercentFromMillivolts(uint16_t batteryMv);

    /** @brief Parses raw I2C data from the AHT20 sensor. */
    bool aht20Parse(const std::array<uint8_t, 6>& data, int8_t& temperatureC, uint8_t& humidityPct);

    /** @brief Sends initialization commands to the AHT20 sensor. */
    bool initAht20(common::IStopToken& token);

    adapters::II2cBus& mI2cBus;                       /**< Reference to I2C master adapter. */
    adapters::IAdcReader& mAdcReader;                 /**< Reference to ADC input adapter. */
    common::IEventQueue& mCoreEventQueue;             /**< Queue for application event dispatch. */
    common::ITaskRunner& mTaskRunner;                 /**< Reference to background task manager. */
    common::IClock& mClock;                           /**< Reference to system time. */
    adapters::IPersistentStorage& mPersistentStorage; /**< Reference to configuration storage. */

    common::TaskHandle mSensorTaskHandle; /**< Handle for the environmental polling task. */
    common::TaskHandle mMicTaskHandle;    /**< Handle for the microphone analysis task. */

    /** @brief Flag indicating if the AHT20 temperature and humidity sensor was successfully
     * initialized. */
    bool mIsAht20Inited;

    /** @brief The calibrated baseline noise floor of the microphone, measured in millivolts (mV).
     */
    int32_t mMicNoiseMv;

    /** * @brief Persistent master override toggle; if false, clap detection is completely disabled
     * system-wide.
     */
    bool mClapFeatureEnabled;

    /** * @brief Runtime state request flag; true if current playback/system conditions meet the
     * criteria to activate the mic processing loop.
     */
    bool mShouldStartClapDet;
};

}  // namespace services
