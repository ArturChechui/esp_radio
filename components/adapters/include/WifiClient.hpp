#pragma once

#include <esp_event.h>
#include <esp_netif.h>
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

    bool init() override;
    void deinit() override;
    bool connect(const common::WifiCredentials& creds) override;
    bool waitForConnection(const uint32_t timeoutMs) override;
    bool disconnect(const uint32_t timeoutMs) override;
    bool startAccessPoint(const common::ProvisioningPortalConfig& cfg) override;
    bool stopAccessPoint() override;
    std::string getApIp() const override;
    bool isConnected() const override;
    std::string getStatus() const override;
    void setStateCallback(common::WifiStateCallback callback) override;
    std::optional<int8_t> tryGetRssiDbm() const override;

   private:
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
                             void* event_data);

    void onEvent(esp_event_base_t event_base, int32_t event_id, void* event_data);
    void notifyStateChange(bool isConnected);
    bool isDnsReady();
    void clearConnectionBits();
    bool ensureWifiReady();
    bool ensureApNetif();
    bool configureStaticApIp() const;
    void resetRuntimeState();

    EventGroupHandle_t mEventGroup;
    common::WifiStateCallback mStateCallback;
    bool mIsInitialized;
    bool mIsStarted;
    bool mIsConnected;
    bool mDisconnectRequested;
    uint16_t mRetryCount;
    std::string mSsid;

    esp_event_handler_instance_t mWifiHandlerInstance;
    esp_event_handler_instance_t mIpHandlerInstance;
    bool mHandlersRegistered;

    esp_netif_t* mStaNetif;
    esp_netif_t* mApNetif;
};

}  // namespace adapters
