#include "WifiClient.hpp"

#include <esp_log.h>
#include <lwip/dns.h>
#include <lwip/ip4_addr.h>
#include <string.h>

#include <algorithm>
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace adapters {
namespace {
constexpr const char* Tag = "WifiClient";
constexpr const char* DefaultSsid = "SetupESPW";

constexpr uint32_t DisconnectTimeoutMs = 1500U;

constexpr uint8_t ApIpA = 192U;
constexpr uint8_t ApIpB = 168U;
constexpr uint8_t ApIpC = 4U;
constexpr uint8_t ApIpD = 1U;
constexpr uint8_t ApMaskA = 255U;
constexpr uint8_t ApMaskB = 255U;
constexpr uint8_t ApMaskC = 255U;
constexpr uint8_t ApMaskD = 0U;

constexpr EventBits_t WifiConnectedBit = BIT0;
constexpr EventBits_t WifiFailedBit = BIT1;
constexpr EventBits_t WifiDisconnectedBit = BIT2;
constexpr uint16_t WifiRetryMax = 5U;

constexpr uint32_t DnsTimeoutMs = 5000U;

static bool modeHasSta(const wifi_mode_t mode) {
    return (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA);
}

static bool modeHasAp(const wifi_mode_t mode) {
    return (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);
}
}  // namespace

WifiClient::WifiClient()
    : mEventGroup(nullptr),
      mStateCallback(nullptr),
      mIsInitialized(false),
      mIsStarted(false),
      mIsConnected(false),
      mDisconnectRequested(false),
      mRetryCount(0U),
      mSsid(),
      mWifiHandlerInstance(),
      mIpHandlerInstance(),
      mHandlersRegistered(false),
      mStaNetif(nullptr),
      mApNetif(nullptr){};

WifiClient::~WifiClient() {
    deinit();
}

bool WifiClient::init() {
    if (mIsInitialized) {
        return true;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(Tag, "esp_netif_init failed: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(Tag, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return false;
    }

    mStaNetif = esp_netif_create_default_wifi_sta();
    if (!mStaNetif) {
        ESP_LOGE(Tag, "Failed to create STA netif");
        return false;
    }

    mEventGroup = xEventGroupCreate();
    if (!mEventGroup) {
        ESP_LOGE(Tag, "Failed to create event group");
        esp_netif_destroy_default_wifi(mStaNetif);
        mStaNetif = nullptr;
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(Tag, "esp_wifi_init failed: %s", esp_err_to_name(err));
        vEventGroupDelete(mEventGroup);
        mEventGroup = nullptr;
        esp_netif_destroy_default_wifi(mStaNetif);
        mStaNetif = nullptr;
        return false;
    }

    err = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiClient::eventHandler, this, &mWifiHandlerInstance);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "Failed to register WIFI_EVENT handler: %s", esp_err_to_name(err));
        (void)esp_wifi_deinit();
        vEventGroupDelete(mEventGroup);
        mEventGroup = nullptr;
        esp_netif_destroy_default_wifi(mStaNetif);
        mStaNetif = nullptr;
        return false;
    }

    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                              &WifiClient::eventHandler, this, &mIpHandlerInstance);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "Failed to register IP_EVENT handler: %s", esp_err_to_name(err));
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, mWifiHandlerInstance);
        (void)esp_wifi_deinit();
        vEventGroupDelete(mEventGroup);
        mEventGroup = nullptr;
        esp_netif_destroy_default_wifi(mStaNetif);
        mStaNetif = nullptr;
        return false;
    }

    mHandlersRegistered = true;
    mIsInitialized = true;
    resetRuntimeState();
    ESP_LOGI(Tag, "WiFi adapter initialized");

    return true;
}

void WifiClient::deinit() {
    if (!mIsInitialized) {
        return;
    }

    ESP_LOGI(Tag, "Deinitializing WiFi");

    (void)disconnect(DisconnectTimeoutMs);
    (void)stopAccessPoint();

    if (mHandlersRegistered) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, mWifiHandlerInstance);
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, mIpHandlerInstance);
        mHandlersRegistered = false;
    }

    const esp_err_t stopErr = esp_wifi_stop();
    if (stopErr != ESP_OK && stopErr != ESP_ERR_WIFI_NOT_INIT &&
        stopErr != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGW(Tag, "esp_wifi_stop failed: %s", esp_err_to_name(stopErr));
    }

    const esp_err_t deinitErr = esp_wifi_deinit();
    if (deinitErr != ESP_OK && deinitErr != ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGW(Tag, "esp_wifi_deinit failed: %s", esp_err_to_name(deinitErr));
    }

    if (mApNetif) {
        esp_netif_destroy_default_wifi(mApNetif);
        mApNetif = nullptr;
    }

    if (mStaNetif) {
        esp_netif_destroy_default_wifi(mStaNetif);
        mStaNetif = nullptr;
    }

    if (mEventGroup) {
        vEventGroupDelete(mEventGroup);
        mEventGroup = nullptr;
    }

    mIsInitialized = false;
    mHandlersRegistered = false;
    resetRuntimeState();
}

bool WifiClient::connect(const common::WifiCredentials& creds) {
    if (creds.ssid.empty()) {
        ESP_LOGE(Tag, "SSID is empty");
        return false;
    }

    if (!ensureWifiReady()) {
        return false;
    }

    const esp_err_t stopErr = esp_wifi_stop();
    if (stopErr != ESP_OK && stopErr != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGE(Tag, "esp_wifi_stop before STA connect failed: %s", esp_err_to_name(stopErr));
        return false;
    }
    mIsStarted = false;

    clearConnectionBits();
    mDisconnectRequested = false;
    mRetryCount = 0U;
    mIsConnected = false;

    wifi_config_t cfg{};
    strncpy(reinterpret_cast<char*>(cfg.sta.ssid), creds.ssid.c_str(), sizeof(cfg.sta.ssid) - 1U);
    strncpy(reinterpret_cast<char*>(cfg.sta.password), creds.password.c_str(),
            sizeof(cfg.sta.password) - 1U);
    cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    cfg.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    auto doStaBringup = [this, &cfg]() -> esp_err_t {
        esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
        if (err != ESP_OK) {
            return err;
        }

        err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
        if (err != ESP_OK) {
            return err;
        }

        err = esp_wifi_start();
        if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
            return err;
        }
        mIsStarted = true;

        err = esp_wifi_set_ps(WIFI_PS_NONE);
        if (err != ESP_OK) {
            return err;
        }

        return esp_wifi_connect();
    };

    esp_err_t err = doStaBringup();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGW(Tag, "WiFi driver not initialized during STA connect, rebuilding state");
        deinit();
        if (!ensureWifiReady()) {
            return false;
        }
        err = doStaBringup();
    }

    if (err != ESP_OK) {
        ESP_LOGE(Tag, "STA connect bringup failed: %s", esp_err_to_name(err));
        return false;
    }

    mSsid = creds.ssid;
    ESP_LOGI(Tag, "Connecting to \"%s\"...", mSsid.c_str());

    return true;
}

bool WifiClient::waitForConnection(const uint32_t timeoutMs) {
    if (!mIsInitialized || !mIsStarted || !mEventGroup) {
        ESP_LOGE(Tag, "WiFi not ready for waitForConnection");
        return false;
    }

    wifi_mode_t mode{};
    const esp_err_t modeErr = esp_wifi_get_mode(&mode);
    if (modeErr != ESP_OK || !modeHasSta(mode)) {
        ESP_LOGE(Tag, "waitForConnection called while not in STA mode");
        return false;
    }

    ESP_LOGI(Tag, "Waiting for WiFi connection (timeout: %lums)...", timeoutMs);

    const EventBits_t bits = xEventGroupWaitBits(mEventGroup, WifiConnectedBit | WifiFailedBit,
                                                 pdFALSE, pdFALSE, pdMS_TO_TICKS(timeoutMs));

    if (bits & WifiConnectedBit) {
        ESP_LOGI(Tag, "WiFi connected");
        mIsConnected = true;

        ip_addr_t dnsIp{};
        ipaddr_aton("8.8.8.8", &dnsIp);
        dns_setserver(0, &dnsIp);

        ipaddr_aton("8.8.4.4", &dnsIp);
        dns_setserver(1, &dnsIp);

        // TODO: make min(remaining, DnsTimeoutMs)
        uint32_t dnsWaitMs = 0U;
        while (!isDnsReady() && dnsWaitMs < DnsTimeoutMs) {
            vTaskDelay(pdMS_TO_TICKS(100U));
            dnsWaitMs += 100U;
        }

        if (!isDnsReady()) {
            ESP_LOGW(Tag, "DNS not ready, proceeding anyway");
        }

        return true;
    }

    if (bits & WifiFailedBit) {
        ESP_LOGE(Tag, "WiFi connection failed");
    } else {
        ESP_LOGE(Tag, "WiFi connection timed out");
    }
    mIsConnected = false;
    return false;
}

bool WifiClient::disconnect(const uint32_t timeoutMs) {
    if (!mIsInitialized) {
        return true;
    }

    wifi_mode_t mode{};
    const esp_err_t modeErr = esp_wifi_get_mode(&mode);
    if (modeErr == ESP_ERR_WIFI_NOT_INIT) {
        resetRuntimeState();
        return true;
    } else if (modeErr != ESP_OK) {
        ESP_LOGW(Tag, "esp_wifi_get_mode failed in disconnect: %s", esp_err_to_name(modeErr));
        return false;
    } else if (!modeHasSta(mode)) {
        return true;
    }

    clearConnectionBits();
    mDisconnectRequested = true;

    const esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_STARTED && err != ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGE(Tag, "esp_wifi_disconnect failed: %s", esp_err_to_name(err));
        mDisconnectRequested = false;
        return false;
    }

    (void)xEventGroupWaitBits(mEventGroup, WifiDisconnectedBit, pdFALSE, pdFALSE,
                              pdMS_TO_TICKS(timeoutMs));

    mRetryCount = 0;
    mIsConnected = false;
    mDisconnectRequested = false;
    notifyStateChange(false);
    ESP_LOGI(Tag, "WiFi disconnected");

    return true;
}

bool WifiClient::startAccessPoint(const common::ProvisioningPortalConfig& cfg) {
    if (!ensureWifiReady()) {
        return false;
    }
    if (!ensureApNetif()) {
        return false;
    }

    const bool wasConnected = mIsConnected;

    const esp_err_t stopErr = esp_wifi_stop();
    if (stopErr != ESP_OK && stopErr != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGE(Tag, "esp_wifi_stop before AP start failed: %s", esp_err_to_name(stopErr));
        return false;
    }
    mIsStarted = false;

    if (!configureStaticApIp()) {
        return false;
    }

    wifi_config_t apCfg{};
    const std::string apSsid = cfg.apSsid.empty() ? DefaultSsid : cfg.apSsid;
    const size_t ssidLen = std::min(apSsid.size(), sizeof(apCfg.ap.ssid) - 1U);
    memcpy(apCfg.ap.ssid, apSsid.c_str(), ssidLen);
    apCfg.ap.ssid[ssidLen] = '\0';
    apCfg.ap.ssid_len = static_cast<uint8_t>(ssidLen);
    apCfg.ap.channel = cfg.channel;
    apCfg.ap.max_connection = cfg.maxConnections;
    apCfg.ap.pmf_cfg.required = false;

    if (cfg.apPassword.empty()) {
        apCfg.ap.authmode = WIFI_AUTH_OPEN;
    } else if (cfg.apPassword.size() < 8U) {
        ESP_LOGW(Tag, "AP password too short, switching to open AP mode");
        apCfg.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        const size_t passLen = std::min(cfg.apPassword.size(), sizeof(apCfg.ap.password) - 1U);
        memcpy(apCfg.ap.password, cfg.apPassword.c_str(), passLen);
        apCfg.ap.password[passLen] = '\0';
        apCfg.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    auto doApBringup = [this, &apCfg]() -> esp_err_t {
        esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (err != ESP_OK) {
            return err;
        }
        err = esp_wifi_set_config(WIFI_IF_AP, &apCfg);
        if (err != ESP_OK) {
            return err;
        }
        return esp_wifi_start();
    };

    esp_err_t err = doApBringup();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGW(Tag, "WiFi driver not initialized during AP start, rebuilding state");
        deinit();
        if (!ensureWifiReady() || !ensureApNetif() || !configureStaticApIp()) {
            return false;
        }
        err = doApBringup();
    } else if (err != ESP_OK) {
        ESP_LOGE(Tag, "AP bringup failed: %s", esp_err_to_name(err));
        return false;
    }

    mIsStarted = true;
    mIsConnected = false;
    mSsid.clear();
    mRetryCount = 0U;
    mDisconnectRequested = false;
    clearConnectionBits();

    if (wasConnected) {
        notifyStateChange(false);
    }

    ESP_LOGI(Tag, "AP started. SSID='%s' channel=%u", apSsid.c_str(),
             static_cast<unsigned>(apCfg.ap.channel));

    return true;
}

bool WifiClient::stopAccessPoint() {
    if (!mIsInitialized) {
        return true;
    }

    wifi_mode_t mode{};
    const esp_err_t modeErr = esp_wifi_get_mode(&mode);
    if (modeErr == ESP_ERR_WIFI_NOT_INIT) {
        resetRuntimeState();
        return true;
    } else if (modeErr != ESP_OK) {
        ESP_LOGW(Tag, "esp_wifi_get_mode failed in stopAccessPoint: %s", esp_err_to_name(modeErr));
        return false;
    } else if (!modeHasAp(mode)) {
        return true;
    }

    const esp_err_t stopErr = esp_wifi_stop();
    if (stopErr != ESP_OK && stopErr != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGE(Tag, "esp_wifi_stop failed while stopping AP: %s", esp_err_to_name(stopErr));
        return false;
    }

    mIsStarted = false;
    mIsConnected = false;
    mDisconnectRequested = false;
    mRetryCount = 0U;
    mSsid.clear();
    clearConnectionBits();
    notifyStateChange(false);

    return true;
}

std::string WifiClient::getApIp() const {
    if (!mApNetif) {
        return {};
    }

    esp_netif_ip_info_t ipInfo{};
    if (esp_netif_get_ip_info(mApNetif, &ipInfo) != ESP_OK) {
        return {};
    }

    char ipStr[16] = {0};
    std::snprintf(ipStr, sizeof(ipStr), IPSTR, IP2STR(&ipInfo.ip));

    return std::string(ipStr);
}

bool WifiClient::isConnected() const {
    return mIsConnected;
}

std::string WifiClient::getStatus() const {
    if (!mIsInitialized) {
        return "Not initialized";
    }

    wifi_mode_t mode{};
    if (esp_wifi_get_mode(&mode) == ESP_OK && modeHasAp(mode) && !modeHasSta(mode)) {
        const std::string ip = getApIp();
        return ip.empty() ? "AP active" : ("AP active at http://" + ip);
    }

    if (mIsConnected) {
        return "Connected to " + mSsid;
    } else if (mIsStarted) {
        return "Disconnected";
    }

    return "Initialized";
}

void WifiClient::setStateCallback(common::WifiStateCallback callback) {
    mStateCallback = std::move(callback);
}

void WifiClient::eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
                              void* event_data) {
    auto* self = static_cast<WifiClient*>(arg);
    if (self) {
        self->onEvent(event_base, event_id, event_data);
    }
}

void WifiClient::notifyStateChange(const bool isConnected) {
    if (mIsConnected == isConnected) {
        return;
    }

    mIsConnected = isConnected;
    const auto rssiOpt = tryGetRssiDbm();
    if (mStateCallback) {
        mStateCallback(
            common::WifiState{.isConnected = mIsConnected, .rssi = rssiOpt.value_or(-100)});
    }
}

void WifiClient::onEvent(esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START: {
                ESP_LOGI(Tag, "WIFI_EVENT_STA_START");
                break;
            }
            case WIFI_EVENT_STA_CONNECTED: {
                ESP_LOGI(Tag, "WIFI_EVENT_STA_CONNECTED");
                break;
            }
            case WIFI_EVENT_STA_STOP: {
                ESP_LOGI(Tag, "WIFI_EVENT_STA_STOP");
                mIsStarted = false;
                mIsConnected = false;
                clearConnectionBits();
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED: {
                if (mEventGroup) {
                    xEventGroupClearBits(mEventGroup, WifiConnectedBit);
                    xEventGroupSetBits(mEventGroup, WifiDisconnectedBit);
                }
                mIsConnected = false;

                if (mDisconnectRequested) {
                    ESP_LOGI(Tag, "WIFI_EVENT_STA_DISCONNECTED (requested)");
                    notifyStateChange(false);
                    break;
                }

                if (mRetryCount < WifiRetryMax) {
                    ESP_LOGW(Tag, "WIFI_EVENT_STA_DISCONNECTED (retry %d/%d)", mRetryCount + 1U,
                             WifiRetryMax);
                    const esp_err_t err = esp_wifi_connect();
                    if (err != ESP_OK) {
                        ESP_LOGE(Tag, "esp_wifi_connect retry failed: %s", esp_err_to_name(err));
                        if (mEventGroup) {
                            xEventGroupSetBits(mEventGroup, WifiFailedBit);
                        }
                        notifyStateChange(false);
                        break;
                    }
                    mRetryCount++;
                } else {
                    ESP_LOGE(Tag, "WIFI_EVENT_STA_DISCONNECTED (max retries exceeded)");
                    if (mEventGroup) {
                        xEventGroupSetBits(mEventGroup, WifiFailedBit);
                    }
                    notifyStateChange(false);
                }
                break;
            }
            case WIFI_EVENT_AP_START: {
                ESP_LOGI(Tag, "WIFI_EVENT_AP_START");
                mIsStarted = true;
                break;
            }
            case WIFI_EVENT_AP_STOP: {
                ESP_LOGI(Tag, "WIFI_EVENT_AP_STOP");
                mIsStarted = false;
                break;
            }
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        if (!event) {
            ESP_LOGW(Tag, "IP_EVENT_STA_GOT_IP event data is null");
            return;
        }

        ESP_LOGI(Tag, "IP_EVENT_STA_GOT_IP: " IPSTR, IP2STR(&event->ip_info.ip));

        mRetryCount = 0U;
        if (mEventGroup) {
            xEventGroupClearBits(mEventGroup, WifiFailedBit);
            xEventGroupClearBits(mEventGroup, WifiDisconnectedBit);
            xEventGroupSetBits(mEventGroup, WifiConnectedBit);
        }

        notifyStateChange(true);
    }
}

bool WifiClient::isDnsReady() {
    const ip_addr_t* dns1 = dns_getserver(0);
    const ip_addr_t* dns2 = dns_getserver(1);

    return (dns1 && dns1->u_addr.ip4.addr != 0U) || (dns2 && dns2->u_addr.ip4.addr != 0U);
}

std::optional<int8_t> WifiClient::tryGetRssiDbm() const {
    wifi_ap_record_t ap{};
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        return std::nullopt;
    }
    return ap.rssi;
}

void WifiClient::clearConnectionBits() {
    if (!mEventGroup) {
        return;
    }

    xEventGroupClearBits(mEventGroup, WifiConnectedBit | WifiFailedBit | WifiDisconnectedBit);
}

bool WifiClient::ensureWifiReady() {
    if (mIsInitialized) {
        wifi_mode_t mode{};
        const esp_err_t stateErr = esp_wifi_get_mode(&mode);
        if (stateErr == ESP_ERR_WIFI_NOT_INIT) {
            ESP_LOGW(Tag, "WiFi driver was deinitialized externally, rebuilding adapter state");
            deinit();
        } else if (stateErr != ESP_OK) {
            ESP_LOGW(Tag, "esp_wifi_get_mode failed (%s), rebuilding adapter state",
                     esp_err_to_name(stateErr));
            deinit();
        }
    }

    if (!mIsInitialized) {
        if (!init()) {
            ESP_LOGE(Tag, "WiFi lazy init failed");
            return false;
        }
    }

    return true;
}

bool WifiClient::ensureApNetif() {
    if (mApNetif) {
        return true;
    }

    mApNetif = esp_netif_create_default_wifi_ap();
    if (!mApNetif) {
        ESP_LOGE(Tag, "Failed to create AP netif");
        return false;
    }

    return true;
}

bool WifiClient::configureStaticApIp() const {
    if (!mApNetif) {
        return false;
    }

    esp_err_t err = esp_netif_dhcps_stop(mApNetif);
    if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        ESP_LOGE(Tag, "esp_netif_dhcps_stop failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_netif_ip_info_t ipInfo{};
    IP4_ADDR(&ipInfo.ip, ApIpA, ApIpB, ApIpC, ApIpD);
    IP4_ADDR(&ipInfo.gw, ApIpA, ApIpB, ApIpC, ApIpD);
    IP4_ADDR(&ipInfo.netmask, ApMaskA, ApMaskB, ApMaskC, ApMaskD);
    err = esp_netif_set_ip_info(mApNetif, &ipInfo);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "esp_netif_set_ip_info failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_netif_dhcps_start(mApNetif);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "esp_netif_dhcps_start failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

void WifiClient::resetRuntimeState() {
    mIsStarted = false;
    mIsConnected = false;
    mDisconnectRequested = false;
    mRetryCount = 0U;
    mSsid.clear();
    clearConnectionBits();
}

}  // namespace adapters
