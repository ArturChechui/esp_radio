/**
 * @file WifiClient.hpp
 * @brief Implementation of the IWifiClient interface for ESP32 Wi-Fi.
 *
 * This file contains the WifiClient class, which manages the ESP32 Wi-Fi stack,
 * including event handling, IP acquisition, and Access Point (AP) functionality.
 */

#pragma once

#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include <lwip/dns.h>

#include <optional>

#include "IWifiClient.hpp"

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer implementation classes.
 */
namespace adapters {

/**
 * @class WifiClient
 * @brief Concrete implementation of a Wi-Fi controller for ESP32.
 *
 * This class handles the complexity of the ESP-IDF Wi-Fi driver. It supports
 * connecting to external routers (Station mode) and hosting a local configuration
 * network (AP mode). It uses FreeRTOS event groups to synchronize connection states
 * and provides status feedback through callbacks.
 */
class WifiClient : public IWifiClient {
   public:
    /**
     * @brief Constructs a new WifiClient object.
     * Initializes internal state flags and event group handles.
     */
    WifiClient();

    /**
     * @brief Destroys the WifiClient object.
     * Ensures event handlers are removed and resources are cleaned up.
     */
    ~WifiClient() override;

    /**
     * @brief Initializes the underlying TCP/IP stack and Wi-Fi peripheral.
     * @return true if initialization was successful, false otherwise.
     */
    bool init() override;

    /**
     * @brief Stops the Wi-Fi hardware and releases associated system resources.
     */
    void deinit() override;

    /**
     * @brief Starts the connection process to a Wi-Fi Access Point.
     * @param creds Structure containing the SSID and password.
     * @return true if the connection command was successfully issued.
     */
    bool connect(const common::WifiCredentials& creds) override;

    /**
     * @brief Blocks the calling task until a connection is established or a timeout occurs.
     * @param timeoutMs Maximum time to wait for the "Connected" state in milliseconds.
     * @return true if connected successfully, false on timeout or failure.
     */
    bool waitForConnection(const uint32_t timeoutMs) override;

    /**
     * @brief Disconnects from the current network and stops the Wi-Fi station.
     * @param timeoutMs Time to wait for the disconnection process to complete.
     * @return true if successfully disconnected.
     */
    bool disconnect(const uint32_t timeoutMs) override;

    /**
     * @brief Configures and starts the local Wi-Fi Access Point.
     * @param cfg Configuration for the AP, including SSID and optional password.
     * @return true if the AP started successfully.
     */
    bool startAccessPoint(const common::ProvisioningPortalConfig& cfg) override;

    /**
     * @brief Stops the local Access Point and clears associated netifs.
     * @return true if successfully stopped.
     */
    bool stopAccessPoint() override;

    /**
     * @brief Retrieves the local IP address of the device when in Access Point mode.
     * @return A string containing the gateway IP (usually "192.168.4.1").
     */
    std::string getApIp() const override;

    /**
     * @brief Checks if the station is currently connected to a network and has an IP.
     * @return true if connected, false otherwise.
     */
    bool isConnected() const override;

    /**
     * @brief Gets a human-readable status string of the current Wi-Fi state.
     * @return A string such as "Connected", "Disconnecting", or "Idle".
     */
    std::string getStatus() const override;

    /**
     * @brief Registers a callback to be notified of Wi-Fi state changes (e.g., connect/disconnect).
     * @param callback The function to invoke on state changes.
     */
    void setStateCallback(common::WifiStateCallback callback) override;

    /**
     * @brief Attempts to read the Received Signal Strength Indicator (RSSI).
     * @return An optional containing the RSSI in dBm, or std::nullopt if the station is not
     * connected.
     */
    std::optional<int8_t> tryGetRssiDbm() const override;

   private:
    /**
     * @brief Static entry point for ESP-IDF system events.
     * dispatches calls to the instance's onEvent method.
     */
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
                             void* event_data);

    /**
     * @brief Internal instance handler for Wi-Fi and IP events.
     */
    void onEvent(esp_event_base_t event_base, int32_t event_id, void* event_data);

    /** @brief Triggers the user-provided state callback. */
    void notifyStateChange(bool isConnected);

    /** @brief Checks if at least one DNS server is configured via DHCP. */
    bool isDnsReady();

    /** @brief Clears the internal FreeRTOS event bits related to connectivity. */
    void clearConnectionBits();

    /** @brief Internal helper to ensure the Wi-Fi driver is in a ready state. */
    bool ensureWifiReady();

    /** @brief Internal helper to initialize the AP network interface. */
    bool ensureApNetif();

    /** @brief Sets a static IP for the AP mode to ensure consistent gateway addressing. */
    bool configureStaticApIp() const;

    /** @brief Resets runtime connection counters and state flags. */
    void resetRuntimeState();

    /** @brief FreeRTOS Event Group handle for synchronizing Wi-Fi events. */
    EventGroupHandle_t mEventGroup;

    /** @brief User-provided callback for Wi-Fi status updates. */
    common::WifiStateCallback mStateCallback;

    bool mIsInitialized;       /**< Flag: init() has been called. */
    bool mIsStarted;           /**< Flag: Wi-Fi hardware is powered on. */
    bool mIsConnected;         /**< Flag: Currently has an IP address. */
    bool mDisconnectRequested; /**< Flag: Prevent auto-reconnect during manual disconnect. */
    uint16_t mRetryCount;      /**< Counter for connection retry attempts. */
    std::string mSsid;         /**< Cache of the current/last SSID. */

    /** @brief Handle for the Wi-Fi event handler registration. */
    esp_event_handler_instance_t mWifiHandlerInstance;
    /** @brief Handle for the IP event handler registration. */
    esp_event_handler_instance_t mIpHandlerInstance;

    bool mHandlersRegistered; /**< Flag: Event handlers are registered */

    /** @brief Pointer to the Station network interface. */
    esp_netif_t* mStaNetif{nullptr};
    /** @brief Pointer to the Access Point network interface. */
    esp_netif_t* mApNetif{nullptr};
};

}  // namespace adapters
