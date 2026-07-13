/**
 * @file AppController.hpp
 * @brief Main application controller responsible for event routing and logic coordination.
 *
 * This file contains the AppController class, which implements the primary
 * event-handling logic for the system, acting as the bridge between
 * hardware events, services, and the user interface.
 */

#pragma once

#include <memory>

#include "Events.hpp"
#include "ICommand.hpp"
#include "IEventHandler.hpp"

/**
 * @namespace common
 * @brief Contains shared interfaces and messaging structures.
 */
namespace common {
class IEventQueue;
class IManifestParser;
class IJsonParser;
}  // namespace common

/**
 * @namespace adapters
 * @brief Contains hardware abstraction layer interfaces.
 */
namespace adapters {
class IHttpClient;
class IFileSystem;
class IPersistentStorage;
}  // namespace adapters

/**
 * @namespace services
 * @brief Contains business logic service interfaces.
 */
namespace services {
class IPlayerService;
class IStationRepository;
class IWifiService;
class ISensorService;
class IInputService;
}  // namespace services

/**
 * @namespace core
 * @brief Contains the high-level application logic.
 */
namespace core {

/**
 * @namespace commands
 * @brief Contains command pattern implementations for application actions.
 */
namespace commands {
class ICommand;
}  // namespace commands

class IUiEventSink;

/**
 * @class AppController
 * @brief Central controller that manages the application state and event flow.
 *
 * The AppController implements the IEventHandler interface to process incoming
 * AppEvents. It dispatches events to different "lanes" (UI or Command) and
 * coordinates interactions between high-level services like the PlayerService
 * and WifiService.
 */
class AppController : public common::IEventHandler {
   public:
    /**
     * @brief Constructs an AppController with all required dependencies.
     * * @param wifiService Service for Wi-Fi management.
     * @param playerService Service for audio playback control.
     * @param stationRepository Repository for radio station data.
     * @param sensorService Service for reading environmental/hardware sensors.
     * @param inputService Service for handling user input devices.
     * @param httpClient Adapter for web requests.
     * @param fileSystem Adapter for storage operations.
     * @param jsonParser Utility for JSON serialization.
     * @param uiEventQueue Queue for sending events back to the UI layer.
     * @param persistentStorage Adapter for non-volatile storage operations.
     */
    AppController(services::IWifiService& wifiService, services::IPlayerService& playerService,
                  services::IStationRepository& stationRepository,
                  services::ISensorService& sensorService, services::IInputService& inputService,
                  adapters::IHttpClient& httpClient, adapters::IFileSystem& fileSystem,
                  common::IJsonParser& jsonParser, common::IEventQueue& uiEventQueue,
                  adapters::IPersistentStorage& persistentStorage);

    /** @brief Virtual destructor. */
    ~AppController() override = default;

    /**
     * @brief The primary entry point for all application events.
     * * This method is called whenever an event is pulled from the global queue.
     * @param e The application event to be processed.
     */
    void onEvent(const common::AppEvent& e) override;

   private:
    /**
     * @brief Processes events that trigger system commands (e.g., changing stations).
     * @param e The application event.
     */
    void processCommandLane(const common::AppEvent& e);

    /**
     * @brief Processes events that update the UI state (e.g., volume change notifications).
     * @param e The application event.
     */
    void processUiLane(const common::AppEvent& e);

    /**
     * @brief Determines if an event originated from a physical user interaction.
     * @param e The application event.
     * @return true if the event is a user input event.
     */
    bool isUserInputEvent(const common::AppEvent& e) const;

    /**
     * @brief Checks if an event represents a simple, non-complex action.
     * @param e The application event.
     * @return true if the action is considered simple.
     */
    bool isSimpleAction(const common::AppEvent& e) const;

    services::IWifiService& mWifiService;             /**< Reference to Wi-Fi logic. */
    services::IPlayerService& mPlayerService;         /**< Reference to audio playback logic. */
    services::IStationRepository& mStationRepository; /**< Reference to station storage. */
    services::ISensorService& mSensorService;         /**< Reference to sensor logic. */
    services::IInputService& mInputService;           /**< Reference to input handling. */
    adapters::IHttpClient& mHttpClient;               /**< Reference to HTTP client. */
    adapters::IFileSystem& mFileSystem;               /**< Reference to file system. */
    common::IJsonParser& mJsonParser;                 /**< Reference to JSON utility. */
    common::IEventQueue& mUiEventQueue;               /**< Queue for UI-bound notifications. */
    adapters::IPersistentStorage& mPersistentStorage; /**< Reference to persistent storage. */

    std::unique_ptr<commands::ICommand> mCurrentCmd; /**< Currently executing system command. */
    bool mInputLocked;       /**< Flag to temporarily disable input processing. */
    uint32_t mAutoplayState; /**< Tracks the current autoplay state for playback. */
};

}  // namespace core
