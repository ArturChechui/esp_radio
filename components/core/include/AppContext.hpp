#pragma once

// WifiClient
#include "WifiClient.hpp"
#include "WifiService.hpp"

// Core
#include "AppController.hpp"
#include "EventTask.hpp"

// Adapters
#include "Display.hpp"
#include "GpioInput.hpp"
#include "HttpClient.hpp"
#include "I2cBus.hpp"
#include "I2sBus.hpp"
#include "Mp3Decoder.hpp"

// Common
#include "AudioBufferStats.hpp"
#include "Clock.hpp"
#include "Queue.hpp"
#include "TaskRunner.hpp"

// Services
#include "InputService.hpp"
#include "PlayerService.hpp"
#include "StationRepository.hpp"
#include "UiService.hpp"

// Standard
#include <memory>

namespace core {
class AppContext {
   public:
    AppContext();
    bool init();

   private:
    // WifiClient - must be first to initialize
    std::shared_ptr<adapters::WifiClient> mWifiAdapter;
    std::shared_ptr<services::WifiService> mWifiService;

    // Adapters
    std::unique_ptr<adapters::I2cBus> mI2cBus;
    std::unique_ptr<adapters::I2sBus> mI2sBus;
    std::unique_ptr<adapters::Display> mDisplay;
    std::unique_ptr<adapters::GpioInput> mGpioInput;
    std::unique_ptr<adapters::HttpClient> mHttpClient;
    std::unique_ptr<adapters::Mp3Decoder> mMp3Decoder;

    // Common
    std::unique_ptr<common::TaskRunner> mTaskRunner;
    std::unique_ptr<common::AudioBufferStats> mStats;
    std::unique_ptr<common::Queue<uint32_t>> mQueue;
    std::unique_ptr<common::Clock> mClock;

    // Tasks
    std::unique_ptr<common::EventTask> mUiEventTask;
    std::unique_ptr<common::EventTask> mCoreEventTask;

    // Services
    std::unique_ptr<services::InputService> mInputService;
    std::unique_ptr<services::PlayerService> mPlayerService;
    std::unique_ptr<services::StationRepository> mStationRepository;
    std::unique_ptr<services::UiService> mUiService;

    // Controller
    std::unique_ptr<AppController> mAppController;
};

}  // namespace core
