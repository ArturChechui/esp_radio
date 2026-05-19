/**
 * @file IWifiService.hpp
 * @brief Interface definition for the Wi-Fi management service.
 *
 * This file defines the abstract interface for a service that manages
 * Wi-Fi connectivity, status monitoring, and the provisioning process.
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @namespace services
 * @brief Contains business logic services that coordinate hardware and application state.
 */
namespace services {

/**
 * @class IWifiService
 * @brief Abstract interface for a high-level Wi-Fi management service.
 *
 * This service provides a simplified API for the application to interact with
 * Wi-Fi hardware. It handles the logic for connecting to networks, checking
 * connection status, and triggering the web-based provisioning portal when
 * credentials are missing or incorrect.
 */
class IWifiService {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IWifiService() = default;

    /**
     * @brief Initializes the Wi-Fi service and underlying network hardware.
     * @return true if the Wi-Fi subsystem was successfully initialized, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Attempts to connect to a known Wi-Fi network.
     * @param timeoutMs Maximum time to wait for a successful connection in milliseconds.
     * Defaults to 30000 (30 seconds).
     * @return true if the connection was established within the timeout, false otherwise.
     */
    virtual bool connect(const uint32_t timeoutMs = 30000) = 0;

    /**
     * @brief Checks if the device is currently connected to a Wi-Fi network.
     * @return true if connected and an IP address is assigned, false otherwise.
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Retrieves a human-readable string representing the current Wi-Fi status.
     * @return A string such as "Connected", "Disconnected", or "Connecting...".
     */
    virtual std::string getStatus() const = 0;

    /**
     * @brief Disconnects from the current Wi-Fi network and shuts down the station interface.
     */
    virtual void disconnect() = 0;

    /**
     * @brief Starts the captive portal for Wi-Fi provisioning.
     * * This typically switches the device into Access Point mode to allow
     * users to enter credentials via a web browser.
     * @return true if the portal was successfully started.
     */
    virtual bool startProvisioningPortal() = 0;
};
}  // namespace services
