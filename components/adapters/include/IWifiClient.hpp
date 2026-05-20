/**
 * @file IWifiClient.hpp
 * @brief Interface definition for Wi-Fi connectivity and management.
 *
 * This file defines the abstract interface for a Wi-Fi client, providing
 * methods for connection management, access point hosting, and status monitoring.
 */

#pragma once

#include <functional>
#include <optional>
#include <string>

#include "Types.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation and interface classes.
 */
namespace adapters {

/**
 * @class IWifiClient
 * @brief Abstract interface for a Wi-Fi hardware controller.
 *
 * This interface encapsulates the complexity of the Wi-Fi stack (like ESP-IDF Wi-Fi).
 * It supports standard station mode (connecting to a router) and Access Point (AP)
 * mode (hosting a network), along with asynchronous state callbacks.
 */
class IWifiClient {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IWifiClient() = default;

    /**
     * @brief Initializes the Wi-Fi peripheral and underlying TCP/IP stack.
     * @return true if initialization was successful, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitializes the Wi-Fi driver and releases all network resources.
     */
    virtual void deinit() = 0;

    /**
     * @brief Attempts to connect to a Wi-Fi network using the provided credentials.
     * @param creds Structure containing SSID and password.
     * @return true if the connection process started successfully, false on immediate failure.
     */
    virtual bool connect(const common::WifiCredentials& creds) = 0;

    /**
     * @brief Blocks until the Wi-Fi connection is established or a timeout occurs.
     * @param timeoutMs Maximum time to wait in milliseconds. Defaults to 30000 (30s).
     * @return true if connected within the timeout, false otherwise.
     */
    virtual bool waitForConnection(uint32_t timeoutMs = 30000) = 0;

    /**
     * @brief Disconnects from the current Wi-Fi network.
     * @param timeoutMs Maximum time to wait for clean disconnection. Defaults to 3000 (3s).
     * @return true if successfully disconnected, false otherwise.
     */
    virtual bool disconnect(uint32_t timeoutMs = 3000) = 0;

    /**
     * @brief Starts the Wi-Fi hardware in Access Point (AP) mode.
     * @param cfg Configuration containing AP SSID, password, and channel.
     * @return true if the AP was successfully started, false otherwise.
     */
    virtual bool startAccessPoint(const common::ProvisioningPortalConfig& cfg) = 0;

    /**
     * @brief Stops the local Access Point and returns to an idle or station state.
     * @return true if successfully stopped, false otherwise.
     */
    virtual bool stopAccessPoint() = 0;

    /**
     * @brief Retrieves the IP address assigned to the local Access Point.
     * @return A string containing the gateway IP (e.g., "192.168.4.1").
     */
    virtual std::string getApIp() const = 0;

    /**
     * @brief Checks if the client is currently connected to a Wi-Fi network.
     * @return true if connected and an IP address is assigned, false otherwise.
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Gets a human-readable string representing the current connection status.
     * @return A string such as "Connected", "Disconnected", or "Connecting...".
     */
    virtual std::string getStatus() const = 0;

    /**
     * @brief Sets a callback function to be invoked when the Wi-Fi state changes.
     * @param callback Function to call on events like GotIP, LostIP, or Disconnected.
     */
    virtual void setStateCallback(common::WifiStateCallback callback) = 0;

    /**
     * @brief Attempts to retrieve the current signal strength (RSSI).
     * @return An optional containing the RSSI in dBm, or std::nullopt if not connected.
     */
    virtual std::optional<int8_t> tryGetRssiDbm() const = 0;
};

}  // namespace adapters
