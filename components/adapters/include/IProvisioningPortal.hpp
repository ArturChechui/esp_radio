/**
 * @file IProvisioningPortal.hpp
 * @brief Interface definition for a Wi-Fi provisioning portal.
 *
 * This file defines the abstract interface for managing a provisioning portal,
 * allowing the device to collect Wi-Fi credentials via a temporary Access Point.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "Types.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation and interface classes.
 */
namespace adapters {

/**
 * @class IProvisioningPortal
 * @brief Abstract interface for a Wi-Fi provisioning portal.
 *
 * This interface provides methods to start and stop a web-based provisioning service.
 * It allows the application to configure the Access Point and receive credentials
 * through a callback mechanism once the user submits them via the portal.
 */
class IProvisioningPortal {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IProvisioningPortal() = default;

    /**
     * @brief Starts the provisioning portal with the given configuration.
     * * This usually involves starting a Wi-Fi Access Point and an HTTP server
     * with a captive portal page.
     * * @param cfg Configuration structure containing SSID, password, and other AP settings.
     * @param onSubmit Callback function invoked when the user submits credentials.
     * @return true if the portal was successfully started, false otherwise.
     */
    virtual bool start(const common::ProvisioningPortalConfig& cfg,
                       common::CredentialsCallback onSubmit) = 0;

    /**
     * @brief Stops the provisioning portal and shuts down the Access Point.
     */
    virtual void stop() = 0;

    /**
     * @brief Checks if the provisioning portal is currently active and running.
     * @return true if the portal/AP is active, false otherwise.
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief Retrieves the IP address of the Access Point.
     * @return A string containing the IP address (e.g., "192.168.4.1").
     */
    virtual std::string getApIp() const = 0;
};
}  // namespace adapters
