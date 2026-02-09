#pragma once

#include <esp_event.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include <lwip/dns.h>

#include <optional>

#include "IWifiClient.hpp"

namespace adapters {

class WifiClient : public IWifiClient {
   public:
    WifiClient();
    ~WifiClient() override;

    bool init(const std::string& ssid, const std::string& password) override;
    bool waitForConnection(uint32_t timeoutMs = 30000) override;
    bool isConnected() const override;
    std::string getStatus() const override;
    void setStateCallback(WifiStateCallback callback) override;
    void deinit() override;
    std::optional<int> tryGetRssiDbm() const override;

   private:
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
                             void* event_data);

    void onEvent(esp_event_base_t event_base, int32_t event_id, void* event_data);
    void notifyStateChange(bool isConnected);
    bool isDnsReady();

    EventGroupHandle_t mEventGroup = nullptr;
    WifiStateCallback mStateCallback = nullptr;
    bool mIsInitialized = false;
    bool mIsConnected = false;
    int mRetryCount = 0;
    std::string mSsid;

    static constexpr int WIFI_CONNECTED_BIT = BIT0;
    static constexpr int WIFI_FAILED_BIT = BIT1;
    static constexpr int WIFI_RETRY_MAX = 5;

    esp_event_handler_instance_t mWifiHandlerInstance{};
    esp_event_handler_instance_t mIpHandlerInstance{};
    bool mHandlersRegistered = false;

    esp_netif_t* mStaNetif = nullptr;
    bool mCreatedEventLoop = false;
};

}  // namespace adapters
