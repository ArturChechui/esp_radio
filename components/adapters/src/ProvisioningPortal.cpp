#include "ProvisioningPortal.hpp"

#include <esp_log.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "IWifiClient.hpp"

namespace adapters {
namespace {
constexpr const char* Tag = "ProvisioningPortal";

// Basic guardrail for POST body size from local form.
constexpr size_t MaxFormSizeBytes = 1024U;

/**
 * @brief Embedded Captive Portal HTML Template
 * @note Input Constraint: The parsing logic downstream expects strict standard ASCII
 * (Latin alphabet, standard digits, and basic punctuation symbols). Non-Latin charsets
 * (e.g., Chinese, Cyrillic, Emojis) will fail validation or corrupt flash storage strings.
 * @note Browser Compatibility: Safari (iOS/macOS) is verified for stable frame handling
 * and captive portal automatic close event behaviors.
 */
constexpr const char* PortalHtml = R"HTML(
<!doctype html>
<html>
<head><meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP Radio Setup</title></head>
<body>
  <h2>ESP Radio Wi-Fi Setup</h2>
  <p>Enter your home Wi-Fi name and password.</p>
  <p>Tap <b>Save and Connect</b>.</p>
  <p>This page will close after credentials are saved.</p>
  <p><small>2.4 GHz Wi-Fi is recommended.</small></p>
  <p><small><b>Notice:</b> Only standard Latin characters, numbers, and basic symbols are supported.</small></p>
  <p><small><b>Recommended Browser:</b> Safari is highly recommended for stable setup connection.</small></p>
  <form method="post" action="/wifi">
    <label>SSID</label><br/>
    <input name="ssid" type="text" maxlength="32" required/><br/><br/>
    <label>Password</label><br/>
    <input name="password" type="password" maxlength="63"/><br/><br/>
    <button type="submit">Save and Connect</button>
  </form>
</body>
</html>
)HTML";

/**
 * @brief Captive Portal Success Page Template
 */
constexpr const char* SuccessHtml = R"HTML(
<!doctype html>
<html>
<head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Saved</title></head>
<body>
  <h2>Credentials saved</h2>
  <p>The device will now try to connect to your Wi-Fi.</p>
  <p>You can close this page.</p>
</body>
</html>
)HTML";
}  // namespace

ProvisioningPortal::ProvisioningPortal(IWifiClient& wifiClient)
    : mOnSubmit(nullptr), mHttpServer(nullptr), mWifiClient(wifiClient), mRunning(false) {}

ProvisioningPortal::~ProvisioningPortal() {
    stop();
}

bool ProvisioningPortal::start(const common::ProvisioningPortalConfig& cfg,
                               common::CredentialsCallback onSubmit) {
    if (mRunning) {
        ESP_LOGW(Tag, "Provisioning portal already running");
        return true;
    }

    mOnSubmit = std::move(onSubmit);

    if (!mWifiClient.startAccessPoint(cfg)) {
        ESP_LOGE(Tag, "Failed to start AP");
        mOnSubmit = nullptr;
        return false;
    }

    if (!startWebServer()) {
        (void)mWifiClient.stopAccessPoint();
        mOnSubmit = nullptr;
        return false;
    }

    mRunning = true;
    ESP_LOGI(Tag, "Provisioning portal started at http://%s", getApIp().c_str());

    return true;
}

void ProvisioningPortal::stop() {
    if (!mRunning && !mHttpServer) {
        return;
    }

    stopWebServer();
    (void)mWifiClient.stopAccessPoint();
    mOnSubmit = nullptr;
    mRunning = false;

    ESP_LOGI(Tag, "Provisioning portal stopped");
}

bool ProvisioningPortal::isRunning() const {
    return mRunning;
}

std::string ProvisioningPortal::getApIp() const {
    return mWifiClient.getApIp();
}

esp_err_t ProvisioningPortal::handleRootGet(httpd_req_t* req) {
    auto* self = static_cast<ProvisioningPortal*>(req->user_ctx);
    return self ? self->onRootGet(req) : ESP_FAIL;
}

esp_err_t ProvisioningPortal::handleWifiPost(httpd_req_t* req) {
    auto* self = static_cast<ProvisioningPortal*>(req->user_ctx);
    return self ? self->onWifiPost(req) : ESP_FAIL;
}

esp_err_t ProvisioningPortal::onRootGet(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, PortalHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t ProvisioningPortal::onWifiPost(httpd_req_t* req) {
    // Browser submits form body as URL-encoded text (not JSON),
    // e.g. "ssid=My+Wifi&password=abc%40123".
    if (req->content_len <= 0 || req->content_len > static_cast<int>(MaxFormSizeBytes)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid form payload");
        return ESP_FAIL;
    }

    // Read entire POST body from socket in case it arrives in chunks.
    std::string body(static_cast<size_t>(req->content_len), '\0');
    int totalRead = 0;
    while (totalRead < req->content_len) {
        const int readNow =
            httpd_req_recv(req, (body.data() + totalRead), (req->content_len - totalRead));
        if (readNow == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (readNow <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read body");
            return ESP_FAIL;
        }
        totalRead += readNow;
    }

    // Parse "key=value&key2=value2" and decode '+' / '%xx' escapes.
    common::WifiCredentials creds{};
    if (!parseCredentialsForm(body, creds) || creds.ssid.empty()) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing SSID");
        return ESP_FAIL;
    }

    ESP_LOGI(Tag, "Received WiFi credentials for SSID '%s'", creds.ssid.c_str());
    if (mOnSubmit) {
        mOnSubmit(creds);
    }

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, SuccessHtml, HTTPD_RESP_USE_STRLEN);
}

bool ProvisioningPortal::startWebServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 10240;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&mHttpServer, &config);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "httpd_start failed: %s", esp_err_to_name(err));
        mHttpServer = nullptr;
        return false;
    }

    httpd_uri_t rootUri{};
    rootUri.uri = "/";
    rootUri.method = HTTP_GET;
    rootUri.handler = &ProvisioningPortal::handleRootGet;
    rootUri.user_ctx = this;
    err = httpd_register_uri_handler(mHttpServer, &rootUri);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "Failed to register GET / handler: %s", esp_err_to_name(err));
        stopWebServer();
        return false;
    }

    httpd_uri_t saveUri{};
    saveUri.uri = "/wifi";
    saveUri.method = HTTP_POST;
    saveUri.handler = &ProvisioningPortal::handleWifiPost;
    saveUri.user_ctx = this;
    err = httpd_register_uri_handler(mHttpServer, &saveUri);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "Failed to register POST /wifi handler: %s", esp_err_to_name(err));
        stopWebServer();
        return false;
    }

    return true;
}

void ProvisioningPortal::stopWebServer() {
    if (!mHttpServer) {
        return;
    }

    (void)httpd_stop(mHttpServer);
    mHttpServer = nullptr;
}

std::string ProvisioningPortal::urlDecode(const std::string& src) {
    // Decode standard x-www-form-urlencoded payload:
    // '+' => space, '%HH' => byte.
    std::string decoded;
    decoded.reserve(src.size());

    for (size_t i = 0; i < src.size(); ++i) {
        const char c = src[i];
        if (c == '+') {
            decoded.push_back(' ');
            continue;
        }

        if (c == '%' && (i + 2U) < src.size()) {
            const unsigned char h1 = static_cast<unsigned char>(src[i + 1U]);
            const unsigned char h2 = static_cast<unsigned char>(src[i + 2U]);
            if (std::isxdigit(h1) && std::isxdigit(h2)) {
                const std::string hex = src.substr(i + 1U, 2U);
                decoded.push_back(static_cast<char>(std::strtol(hex.c_str(), nullptr, 16)));
                i += 2U;
                continue;
            }
        }

        decoded.push_back(c);
    }

    return decoded;
}

bool ProvisioningPortal::parseCredentialsForm(const std::string& body,
                                              common::WifiCredentials& out) {
    // Format expected from browser form POST:
    // "ssid=<encoded>&password=<encoded>".
    size_t pos = 0;
    while (pos < body.size()) {
        size_t amp = body.find('&', pos);
        if (amp == std::string::npos) {
            amp = body.size();
        }

        const std::string pair = body.substr(pos, amp - pos);
        const size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            // Both key and value can be URL-escaped, so decode both.
            const std::string key = urlDecode(pair.substr(0, eq));
            const std::string val = urlDecode(pair.substr(eq + 1U));
            if (key == "ssid") {
                out.ssid = val;
            } else if (key == "password") {
                out.password = val;
            }
        }

        pos = amp + 1U;
    }

    return !out.ssid.empty();
}

}  // namespace adapters
