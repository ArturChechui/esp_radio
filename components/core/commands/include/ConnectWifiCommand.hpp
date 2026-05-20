/**
 * @file ConnectWifiCommand.hpp
 * @brief Command for managing the Wi-Fi connection lifecycle.
 *
 * This file contains the ConnectWifiCommand class, which handles connecting to
 * known networks or launching the provisioning portal if no credentials exist.
 */

#pragma once

#include <optional>
#include <string>

#include "Events.hpp"
#include "ICommand.hpp"
#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared messaging and event structures.
 */
namespace common {
class IEventQueue;
}

/**
 * @namespace services
 * @brief Contains business logic service interfaces.
 */
namespace services {
class IWifiService;
class IStationRepository;
}  // namespace services

/**
 * @namespace core::commands
 * @brief Contains concrete command implementations for the application state machine.
 */
namespace core::commands {

/**
 * @class ConnectWifiCommand
 * @brief Command that orchestrates the Wi-Fi connection process.
 *
 * This command handles the logic of checking for stored credentials,
 * attempting a connection, and monitoring state changes. If a connection
 * cannot be established, it triggers the provisioning portal to collect
 * new credentials from the user.
 */
class ConnectWifiCommand : public ICommand {
   public:
    /**
     * @brief Constructs a ConnectWifiCommand.
     * @param wifiService Reference to the Wi-Fi service for connection control.
     * @param uiEventQueue Queue used to send status updates back to the UI.
     */
    ConnectWifiCommand(services::IWifiService& wifiService, common::IEventQueue& uiEventQueue);

    /** @brief Default virtual destructor. */
    ~ConnectWifiCommand() override = default;

    /**
     * @brief Handles events related to the Wi-Fi connection process.
     * @param e The application event to process (e.g., WifiStateChanged or CredentialsReceived).
     */
    void handle(const common::AppEvent& e) override;

    /**
     * @brief Checks if the Wi-Fi connection process (or provisioning) has completed.
     * @return true if the command has finished its lifecycle.
     */
    bool isFinished() override;

   private:
    /**
     * @brief Initiates the connection attempt using the Wi-Fi service.
     */
    void connect();

    /**
     * @brief Internal handler for Wi-Fi state transition events.
     * @param isConnected True if the hardware successfully connected to an AP.
     */
    void onWifiStateChanged(const bool isConnected);

    /**
     * @brief Internal handler for when new credentials are submitted via the portal.
     */
    void onCredsReceived();

   private:
    services::IWifiService& mWifiService; /**< Reference to the Wi-Fi logic service. */
    common::IEventQueue& mUiEventQueue;   /**< Queue for UI-bound notification events. */

    bool mProvisioningPortalStarted; /**< Flag: indicates if the captive portal is active. */
    bool mFinished;                  /**< Flag: indicates if the command execution is complete. */
};

}  // namespace core::commands
