/**
 * @file AppContext.hpp
 * @brief Central registry and lifecycle manager for the application.
 *
 * This file contains the AppContext class, which is responsible for instantiating
 * and initializing all system components in the correct order.
 */

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

/**
 * @namespace core
 * @brief Contains the high-level application logic and context management.
 */
namespace core {

/**
 * @class AppContext
 * @brief Manages the ownership and initialization of all application modules.
 *
 * AppContext serves as a Dependency Injection container. It holds unique pointers
 * to every adapter (hardware), service (business logic), and common utility.
 * The class ensures that hardware is initialized before services that depend on them.
 */
class AppContext {
   public:
    /**
     * @brief Constructs the AppContext.
     * Instantiates all objects but does not perform hardware initialization.
     */
    AppContext();

    /**
     * @brief High-level initialization routine.
     * Calls the internal init procedures in the required sequence.
     * @return true if the entire system was initialized successfully.
     */
    bool init();

    /**
     * @brief Initializes the ESP32 NVS Flash partition.
     * Must be called before any storage or Wi-Fi operations.
     * @return true if successful.
     */
    bool initNvsFlash();

    /**
     * @brief Initializes common utility modules.
     * @return true if successful.
     */
    bool initCommon();

    /**
     * @brief Initializes hardware adapters (I2C, I2S, Display, etc.).
     * @return true if all hardware modules reported success.
     */
    bool initAdapters();

    /**
     * @brief Initializes high-level services and business logic.
     * @return true if all services started successfully.
     */
    bool initServices();

    /**
     * @brief Initializes the core application controller.
     * @return true if successful.
     */
    bool initCore();

   private:
    // --- Adapters (Hardware Layer) ---
    std::unique_ptr<adapters::PersistentStorage> mPersistentStorage;   /**< Key-value storage. */
    std::unique_ptr<adapters::WifiClient> mWifiAdapter;                /**< Wi-Fi driver wrapper. */
    std::unique_ptr<adapters::ProvisioningPortal> mProvisioningPortal; /**< Web config portal. */
    std::unique_ptr<adapters::I2cBus> mI2cBus;                         /**< I2C master bus. */
    std::unique_ptr<adapters::I2sBus> mI2sBus;                         /**< I2S audio bus. */
    std::unique_ptr<adapters::Display> mDisplay;                       /**< Screen driver. */
    std::unique_ptr<adapters::GpioInput> mGpioInput;         /**< Digital input handler. */
    std::unique_ptr<adapters::AdcReader> mAdcReader;         /**< Analog input handler. */
    std::unique_ptr<adapters::HttpClient> mStreamHttpClient; /**< Audio stream client. */
    std::unique_ptr<adapters::HttpClient> mJsonHttpClient;   /**< API/JSON client. */
    std::unique_ptr<adapters::Mp3Decoder> mMp3Decoder;       /**< MP3 to PCM decoder. */
    std::unique_ptr<adapters::FileSystem> mFileSystem;       /**< LittleFS/SPIFFS manager. */

    // --- Common (Utilities Layer) ---
    std::unique_ptr<common::TaskRunner> mTaskRunner;   /**< Background task manager. */
    std::unique_ptr<common::AudioBufferStats> mStats;  /**< Playback statistics. */
    std::unique_ptr<common::Queue<uint32_t>> mQueue;   /**< Global event queue. */
    std::unique_ptr<common::Clock> mClock;             /**< System time and SNTP. */
    std::unique_ptr<common::EventTask> mUiEventTask;   /**< UI processing thread. */
    std::unique_ptr<common::EventTask> mCoreEventTask; /**< Logic processing thread. */
    std::unique_ptr<common::JsonParser> mJsonParser;   /**< JSON serialization helper. */

    // --- Services (Business Logic Layer) ---
    std::unique_ptr<services::WifiService> mWifiService;     /**< High-level Wi-Fi logic. */
    std::unique_ptr<services::InputService> mInputService;   /**< Button/Dial processing. */
    std::unique_ptr<services::PlayerService> mPlayerService; /**< Audio playback management. */
    std::unique_ptr<services::SensorService> mSensorService; /**< Analog sensor processing. */
    std::unique_ptr<services::StationRepository> mStationRepository; /**< Radio station database. */
    std::unique_ptr<services::UiService> mUiService; /**< Display and UI flow logic. */

    // --- Core ---
    std::unique_ptr<core::AppController> mAppController; /**< Main state machine. */
};

}  // namespace core
