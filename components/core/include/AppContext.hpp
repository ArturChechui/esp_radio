#pragma once

// Core
#include "AppController.hpp"

// Adapters
#include "AdcReader.hpp"
#include "Display.hpp"
#include "FileSystem.hpp"
#include "GpioInput.hpp"
#include "HttpClient.hpp"
#include "I2cBus.hpp"
#include "I2sBus.hpp"
#include "Mp3Decoder.hpp"
#include "PersistentStorage.hpp"
#include "ProvisioningPortal.hpp"
#include "WifiClient.hpp"

// Common
#include "AudioBufferStats.hpp"
#include "Clock.hpp"
#include "EventTask.hpp"
#include "JsonParser.hpp"
#include "Queue.hpp"
#include "TaskRunner.hpp"

// Services
#include "InputService.hpp"
#include "PlayerService.hpp"
#include "SensorService.hpp"
#include "StationRepository.hpp"
#include "UiService.hpp"
#include "WifiService.hpp"

// Standard
#include <memory>

namespace core {
class AppContext {
   public:
    AppContext();
    bool init();
    bool initNvsFlash();
    bool initCommon();
    bool initAdapters();
    bool initServices();
    bool initCore();

   private:
    // Adapters
    std::unique_ptr<adapters::PersistentStorage> mPersistentStorage;
    std::unique_ptr<adapters::WifiClient> mWifiAdapter;
    std::unique_ptr<adapters::ProvisioningPortal> mProvisioningPortal;
    std::unique_ptr<adapters::I2cBus> mI2cBus;
    std::unique_ptr<adapters::I2sBus> mI2sBus;
    std::unique_ptr<adapters::Display> mDisplay;
    std::unique_ptr<adapters::GpioInput> mGpioInput;
    std::unique_ptr<adapters::AdcReader> mAdcReader;
    std::unique_ptr<adapters::HttpClient> mStreamHttpClient;
    std::unique_ptr<adapters::HttpClient> mJsonHttpClient;
    std::unique_ptr<adapters::Mp3Decoder> mMp3Decoder;
    std::unique_ptr<adapters::FileSystem> mFileSystem;

    // Common
    std::unique_ptr<common::TaskRunner> mTaskRunner;
    std::unique_ptr<common::AudioBufferStats> mStats;
    std::unique_ptr<common::Queue<uint32_t>> mQueue;
    std::unique_ptr<common::Clock> mClock;
    std::unique_ptr<common::EventTask> mUiEventTask;
    std::unique_ptr<common::EventTask> mCoreEventTask;
    std::unique_ptr<common::JsonParser> mJsonParser;

    // Services
    std::unique_ptr<services::WifiService> mWifiService;
    std::unique_ptr<services::InputService> mInputService;
    std::unique_ptr<services::PlayerService> mPlayerService;
    std::unique_ptr<services::StationRepository> mStationRepository;
    std::unique_ptr<services::UiService> mUiService;
    std::unique_ptr<services::SensorService> mSensorService;

    // Core
    std::unique_ptr<AppController> mAppController;
};

}  // namespace core
