#include "WifiClient.hpp"

#include <esp_log.h>
#include <esp_netif.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <nvs_flash.h>
#include <string.h>

namespace adapters {

static const char* TAG = "WifiClient";

WifiClient::WifiClient() = default;

WifiClient::~WifiClient() {
    deinit();
}

bool WifiClient::init(const std::string& ssid, const std::string& password) {
    if (mIsInitialized) {
        ESP_LOGW(TAG, "WiFi already initialized");
        return true;
    }

    mSsid = ssid;

    ESP_ERROR_CHECK(esp_netif_init());

    esp_err_t ret = esp_event_loop_create_default();
    if (ret == ESP_OK) {
        mCreatedEventLoop = true;
    } else if (ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop creation failed: %s", esp_err_to_name(ret));
        return false;
    }

    mStaNetif = esp_netif_create_default_wifi_sta();
    if (!mStaNetif) {
        ESP_LOGE(TAG, "Failed to create STA netif");
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    mEventGroup = xEventGroupCreate();
    if (!mEventGroup) {
        ESP_LOGE(TAG, "Failed to create event group");
        return false;
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiClient::eventHandler, this, &mWifiHandlerInstance));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiClient::eventHandler, this, &mIpHandlerInstance));

    mHandlersRegistered = true;

    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid.c_str(),
            sizeof(wifi_config.sta.ssid) - 1);
    strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password.c_str(),
            sizeof(wifi_config.sta.password) - 1);

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    mIsInitialized = true;

    ESP_LOGI(TAG, "WiFi adapter initialized, connecting to \"%s\"...", ssid.c_str());

    return true;
}

bool WifiClient::waitForConnection(uint32_t timeoutMs) {
    if (!mIsInitialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return false;
    }

    ESP_LOGI(TAG, "Waiting for WiFi connection (timeout: %lums)...", timeoutMs);

    EventBits_t bits = xEventGroupWaitBits(mEventGroup, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                           pdFALSE, pdFALSE, pdMS_TO_TICKS(timeoutMs));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected!");
        mIsConnected = true;

        ip_addr_t dns_ip;
        ipaddr_aton("8.8.8.8", &dns_ip);
        dns_setserver(0, &dns_ip);

        ipaddr_aton("8.8.4.4", &dns_ip);
        dns_setserver(1, &dns_ip);

        ESP_LOGI(TAG, "Set DNS servers to 8.8.8.8 and 8.8.4.4");

        uint32_t dnsWaitMs = 0;
        const uint32_t dnsTimeoutMs = 5000;
        while (!isDnsReady() && dnsWaitMs < dnsTimeoutMs) {
            vTaskDelay(pdMS_TO_TICKS(100));
            dnsWaitMs += 100;
        }

        if (!isDnsReady()) {
            ESP_LOGW(TAG, "DNS not ready, but proceeding anyway");
        } else {
            ESP_LOGI(TAG, "DNS is ready!");
        }

        return true;
    }

    ESP_LOGE(TAG, "❌ WiFi connection failed");
    mIsConnected = false;
    return false;
}

bool WifiClient::isConnected() const {
    return mIsConnected;
}

std::string WifiClient::getStatus() const {
    if (!mIsInitialized) {
        return "Not initialized";
    }
    if (mIsConnected) {
        return "Connected to " + mSsid;
    }
    return "Disconnected";
}

void WifiClient::setStateCallback(WifiStateCallback callback) {
    mStateCallback = callback;
}

void WifiClient::deinit() {
    if (!mIsInitialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing WiFi");

    if (mHandlersRegistered) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, mWifiHandlerInstance);
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, mIpHandlerInstance);
        mHandlersRegistered = false;
    }

    esp_netif_destroy_default_wifi(mStaNetif);
    mStaNetif = nullptr;

    esp_wifi_stop();
    esp_wifi_deinit();

    if (mEventGroup) {
        vEventGroupDelete(mEventGroup);
        mEventGroup = nullptr;
    }

    mIsInitialized = false;
    mIsConnected = false;
}

void WifiClient::eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
                              void* event_data) {
    WifiClient* self = static_cast<WifiClient*>(arg);
    if (self) {
        self->onEvent(event_base, event_id, event_data);
    }
}

void WifiClient::notifyStateChange(bool isConnected) {
    if (mStateCallback) {
        WifiStateChangedEvent event{isConnected, mSsid};
        mStateCallback(event);
    }
}

void WifiClient::onEvent(esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                if (mRetryCount < WIFI_RETRY_MAX) {
                    ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED (retry %d/%d)", mRetryCount + 1,
                             WIFI_RETRY_MAX);
                    esp_wifi_connect();
                    mRetryCount++;
                } else {
                    ESP_LOGE(TAG, "WIFI_EVENT_STA_DISCONNECTED (max retries exceeded)");
                    xEventGroupSetBits(mEventGroup, WIFI_FAILED_BIT);
                    mIsConnected = false;
                    notifyStateChange(false);
                }
                break;
            }

            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP: " IPSTR, IP2STR(&event->ip_info.ip));

        mRetryCount = 0;
        xEventGroupSetBits(mEventGroup, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(mEventGroup, WIFI_FAILED_BIT);
        mIsConnected = true;

        //  Notify all listeners that WiFi is now available
        notifyStateChange(true);
    }
}

bool WifiClient::isDnsReady() {
    const ip_addr_t* dns1 = dns_getserver(0);
    const ip_addr_t* dns2 = dns_getserver(1);

    // At least one DNS server should be set (not 0.0.0.0)
    return (dns1 && dns1->u_addr.ip4.addr != 0) || (dns2 && dns2->u_addr.ip4.addr != 0);
}

std::optional<int> WifiClient::tryGetRssiDbm() const {
    wifi_ap_record_t ap{};
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        return std::nullopt;  // not connected / no info
    }
    return static_cast<int>(ap.rssi);
}

}  // namespace adapters
