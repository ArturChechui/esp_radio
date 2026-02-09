#pragma once

#include <functional>
#include <optional>
#include <string>

namespace adapters {

/**
 * @brief WiFi connection state changed event
 */
struct WifiStateChangedEvent {
    bool isConnected;
    std::string ssid;
};

/**
 * @brief Callback for WiFi state changes
 */
using WifiStateCallback = std::function<void(const WifiStateChangedEvent&)>;

class IWifiClient {
   public:
    virtual ~IWifiClient() = default;

    /**
     * @brief Initialize WiFi hardware
     * @param ssid WiFi SSID
     * @param password WiFi password
     * @return true if init succeeded
     */
    virtual bool init(const std::string& ssid, const std::string& password) = 0;

    /**
     * @brief Wait for WiFi connection with timeout
     * @param timeoutMs Timeout in milliseconds
     * @return true if connected within timeout
     */
    virtual bool waitForConnection(uint32_t timeoutMs = 30000) = 0;

    /**
     * @brief Check if WiFi is connected
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Get connection status string
     */
    virtual std::string getStatus() const = 0;

    /**
     * @brief Register callback for WiFi state changes
     * @param callback Function to call when WiFi state changes
     */
    virtual void setStateCallback(WifiStateCallback callback) = 0;

    /**
     * @brief Cleanup
     */
    virtual void deinit() = 0;

    virtual std::optional<int> tryGetRssiDbm() const = 0;
};

}  // namespace adapters
