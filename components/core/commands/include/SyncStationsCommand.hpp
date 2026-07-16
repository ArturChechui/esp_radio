/**
 * @file SyncStationsCommand.hpp
 * @brief Command for synchronizing the radio station database with a remote server.
 *
 * This file contains the SyncStationsCommand class, which manages the multi-step
 * process of checking for updates, downloading new station lists, and
 * updating local storage without interrupting system stability.
 */

#pragma once

#include <string>

#include "ICommand.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer interfaces.
 */
namespace adapters {
class IHttpClient;
class IFileSystem;
}  // namespace adapters

/**
 * @namespace common
 * @brief Contains shared messaging, parsing, and event structures.
 */
namespace common {
class IEventQueue;
class IJsonParser;
}  // namespace common

/**
 * @namespace services
 * @brief Contains business logic service interfaces.
 */
namespace services {
class IPlayerService;
class IStationRepository;
class IWifiService;
}  // namespace services

/**
 * @namespace core::commands
 * @brief Contains concrete command implementations for the application state machine.
 */
namespace core::commands {

/**
 * @class SyncStationsCommand
 * @brief Command that performs an atomic update of the local station database.
 *
 * This command coordinates a complex workflow:
 * 1. Stops audio playback to free up memory and network bandwidth.
 * 2. Compares a remote manifest with the local version.
 * 3. Downloads the new station JSON if a difference is found.
 * 4. Validates and saves the new data using an atomic "Stage-and-Swap" pattern
 * to prevent data corruption in case of a power failure.
 */
class SyncStationsCommand : public ICommand {
   public:
    /**
     * @brief Constructs a SyncStationsCommand with all necessary dependencies.
     * @param playerService Used to halt playback during sync.
     * @param wifiService Used to ensure network connectivity.
     * @param httpClient Used to fetch the remote manifest and station list.
     * @param fileSystem Used to persist the downloaded data.
     * @param jsonParser Used to validate and compare manifest data.
     * @param stationRepository Used to reload the database after sync.
     * @param uiEventQueue Used to notify the UI of sync progress or errors.
     */
    SyncStationsCommand(services::IPlayerService& playerService,
                        services::IWifiService& wifiService, adapters::IHttpClient& httpClient,
                        adapters::IFileSystem& fileSystem, common::IJsonParser& jsonParser,
                        services::IStationRepository& stationRepository,
                        common::IEventQueue& uiEventQueue);

    /** @brief Default virtual destructor. */
    ~SyncStationsCommand() override = default;

    /**
     * @brief Processes events related to the synchronization lifecycle.
     * @param e The application event to handle.
     */
    void handle(const common::AppEvent& e) override;

    /**
     * @brief Checks if the synchronization process has reached a terminal state.
     * @return true if the sync is finished (success or failure).
     */
    bool isFinished() override;

    /**
     * @brief Retrieves the type of the command for identification and routing purposes.
     * @return The type of the command.
     */
    common::CommandType getCmdType() override;

    /**
     * @brief Retrieves the result of the command's execution.
     * @return An optional boolean indicating success (true), failure (false), or nullopt if the
     * result is not yet available since the command is still executing.
     */
    std::optional<bool> getResult() override;

   private:
    /**
     * @brief Requests the PlayerService to stop playback if it is currently active.
     * @return true if the stop request was issued, false if already stopped.
     */
    bool stopPlaybackIfStarted();

    /**
     * @brief Internal handler for playback status changes during the sync process.
     * @param s The current playback status.
     */
    void onPlaybackStatus(common::PlaybackStatus s);

    /**
     * @brief Executes the high-level synchronization logic.
     * @return true if the sync process completed successfully.
     */
    bool runSync();

    /**
     * @brief Compares the remote manifest version with the local one.
     * @param[out] outDifferent Set to true if the remote manifest indicates an update is available.
     * @return true if the check was performed successfully.
     */
    bool manifestsDifferent(bool& outDifferent);

    /**
     * @brief Downloads the station list and performs basic JSON validation.
     * @param[out] outStationsJson String to store the downloaded JSON content.
     * @return true if download and validation succeeded.
     */
    bool downloadAndValidateStations(std::string& outStationsJson);

    /**
     * @brief Stages the new files and performs an atomic swap with existing files.
     * @param manifestJson The new manifest content.
     * @param stationsJson The new stations content.
     * @return true if files were swapped successfully.
     */
    bool stageAndSwapFiles(const std::string& manifestJson, const std::string& stationsJson);

    /**
     * @brief Helper to replace a file atomically using a backup/temporary strategy.
     * @param livePath The path to the file currently in use.
     * @param tmpPath The path to the new temporary file.
     * @param backupPath The path where the current file is backed up before replacement.
     * @return true if the swap was completed without error.
     */
    bool replaceFileAtomically(const std::string& livePath, const std::string& tmpPath,
                               const std::string& backupPath);

    /**
     * @brief Finalizes the command state and notifies the UI.
     * @param success true if the sync completed successfully, false otherwise.
     */
    void finish(const bool success);

    services::IPlayerService& mPlayerService;         /**< Reference to audio playback control. */
    services::IWifiService& mWifiService;             /**< Reference to Wi-Fi status logic. */
    adapters::IHttpClient& mHttpClient;               /**< Reference to HTTP download adapter. */
    adapters::IFileSystem& mFileSystem;               /**< Reference to storage adapter. */
    common::IJsonParser& mJsonParser;                 /**< Reference to JSON parsing utility. */
    services::IStationRepository& mStationRepository; /**< Reference to station data storage. */
    common::IEventQueue& mUiEventQueue; /**< Queue for UI-bound notification events. */

    bool mStarted;                   /**< Flag: indicates if the sync logic has begun. */
    std::optional<bool> mResult;     /**< Flag: indicates the result of the command execution. */
    std::string mRemoteManifestJson; /**< Cache for the downloaded manifest data. */
};

}  // namespace core::commands
