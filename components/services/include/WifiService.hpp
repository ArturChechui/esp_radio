/**
 * @file WifiService.hpp
 * @brief Implementation of the IWifiService interface for Wi-Fi management.
 *
 * This file contains the WifiService class, which manages connection state,
 * credentials, and the background task for signal monitoring and provisioning.
 */

#pragma once

#include <string>

#include "IWifiService.hpp"
#include "Types.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and system abstraction layer interfaces.
 */
namespace adapters {
class IPersistentStorage;
class IWifiClient;
class IProvisioningPortal;
}  // namespace adapters

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {
class ITaskRunner;
class IEventQueue;
}  // namespace common

/**
 * @namespace services
 * @brief Contains business logic service implementations.
 */
namespace services {

/**
 * @class WifiService
 * @brief Concrete implementation of Wi-Fi management logic.
 *
 * WifiService handles the lifecycle of Wi-Fi connectivity. It attempts to
 * connect using stored credentials, monitors the signal quality via a
 * background task, and launches the provisioning portal if a connection
 * cannot be established.
 */
class WifiService final : public IWifiService {
   public:
    /**
     * @brief Constructs a WifiService with its required dependencies.
     * @param wifiClient Reference to the low-level Wi-Fi hardware adapter.
     * @param provisioningPortal Reference to the captive portal manager.
     * @param taskRunner The task runner used to manage background monitoring.
     * @param coreEventQueue Queue for dispatching system-wide Wi-Fi events.
     * @param persistentStorage Storage adapter for saving/loading credentials.
     */
    explicit WifiService(adapters::IWifiClient& wifiClient,
                         adapters::IProvisioningPortal& provisioningPortal,
                         common::ITaskRunner& taskRunner, common::IEventQueue& coreEventQueue,
                         adapters::IPersistentStorage& persistentStorage);

    /** @brief Default virtual destructor. */
    ~WifiService() override = default;

    /**
     * @brief Initializes the Wi-Fi adapter and loads stored credentials.
     * @return true if initialization succeeded.
     */
    bool init() override;

    /**
     * @brief Attempts to establish a connection to a Wi-Fi network.
     * @param timeoutMs Maximum time to wait for a successful connection.
     * @return true if connected successfully within the timeout.
     */
    bool connect(const uint32_t timeoutMs) override;

    /**
     * @brief Checks if the station is currently connected.
     * @return true if connected with an IP address.
     */
    bool isConnected() const override;

    /**
     * @brief Returns a human-readable status of the Wi-Fi connection.
     * @return Status string (e.g., "Connected", "Idle").
     */
    std::string getStatus() const override;

    /**
     * @brief Disconnects from the current network and stops background monitoring.
     */
    void disconnect() override;

    /**
     * @brief Starts the web-based provisioning portal.
     * @return true if the portal started successfully.
     */
    bool startProvisioningPortal() override;

   private:
    /**
     * @brief Static wrapper for the signal monitoring task entry point.
     */
    static common::StepResult signalStepFn(void* arg, common::IStopToken& token);

    /**
     * @brief Background task logic for monitoring signal strength and connection status.
     */
    common::StepResult signalStep(common::IStopToken& token);

    /**
     * @brief Internal handler for Wi-Fi state transition events from the adapter.
     * @param data The new Wi-Fi state data.
     */
    void onWifiStateChanged(const common::WifiState& data);

    /**
     * @brief Internal handler for credentials received via the provisioning portal.
     * @param data The new Wi-Fi credentials.
     */
    void onProvisioningCredentials(const common::WifiCredentials& data);

    adapters::IWifiClient& mWifiAdapter;                /**< Reference to Wi-Fi hardware adapter. */
    adapters::IProvisioningPortal& mProvisioningPortal; /**< Reference to provisioning portal. */
    common::ITaskRunner& mTaskRunner;                 /**< Reference to background task manager. */
    common::IEventQueue& mCoreEventQueue;             /**< Queue for system event dispatch. */
    adapters::IPersistentStorage& mPersistentStorage; /**< Reference to configuration storage. */

    uint8_t mLastBars;                    /**< Last recorded signal strength in 'bars'. */
    common::TaskHandle mSignalTaskHandle; /**< Handle for the signal monitoring thread. */
    common::WifiCredentials mWifiCreds;   /**< Cache of the active Wi-Fi credentials. */
    bool mProvisioningRunning;            /**< Flag: Indicates if the portal is active. */
};

}  // namespace services
