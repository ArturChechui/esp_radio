#include "HttpClient.hpp"

#include <esp_crt_bundle.h>
#include <esp_err.h>
#include <esp_log.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

namespace adapters {
namespace {
constexpr const char* Tag = "HttpClient";
constexpr uint16_t DownloadChunkSize = 1024U;   // 1 KB
constexpr size_t MaxDownloadSize = 4U * 1024U;  // 4 KB
constexpr int HttpStatusOkThreshold = 400;      // HTTP status codes below this are OK
}  // namespace

HttpClient::HttpClient(const size_t& bufferSize)
    : mHttpClient(nullptr), mUrl(""), mTimeoutMs(DefaultStreamTimeoutMs), mBufferSize(bufferSize) {}

HttpClient::~HttpClient() {
    closeStream();
}

bool HttpClient::openStream(const std::string& url, const uint32_t& timeoutMs) {
    mUrl = url;
    mTimeoutMs = timeoutMs;

    if (!openConnection()) {
        return false;
    }

    ESP_LOGI(Tag, "Stream opened successfully");
    return true;
}

// TODO: add proper ICY handling - separate class?
int HttpClient::readStream(uint8_t* buffer, const size_t& size) {
    if (!mHttpClient) {
        ESP_LOGW(Tag, "Stream not open");
        return -1;
    }

    int bytesRead =
        esp_http_client_read(mHttpClient, reinterpret_cast<char*>(buffer), static_cast<int>(size));

    if (bytesRead == -ESP_ERR_HTTP_EAGAIN) {  // == -0x7007 == -28679
        return 0;                             // "no data now, try later"
    }

    return bytesRead;
}

void HttpClient::closeStream() {
    closeConnection();
}

bool HttpClient::download(const std::string& url, std::string& result, const uint32_t& timeoutMs) {
    mUrl = url;
    mTimeoutMs = timeoutMs;

    if (!openConnection()) {
        return false;
    }

    result.clear();
    result.reserve(MaxDownloadSize);

    uint8_t buffer[DownloadChunkSize];
    int bytesRead = 0;
    size_t totalBytes = 0;
    while (true) {
        bytesRead =
            esp_http_client_read(mHttpClient, reinterpret_cast<char*>(buffer), sizeof(buffer));
        if (bytesRead < 0) {
            ESP_LOGE(Tag, "Download failed: read error %d", bytesRead);
            closeConnection();
            return false;
        }

        if (bytesRead == 0) {
            break;
        }

        // Safety: prevent downloading huge files
        if (totalBytes + bytesRead > MaxDownloadSize) {
            ESP_LOGE(Tag, "Download too large (>%zu bytes)", MaxDownloadSize);
            closeConnection();
            return false;
        }

        result.append(reinterpret_cast<const char*>(buffer), bytesRead);
        totalBytes += bytesRead;

        ESP_LOGI(Tag, "Downloaded %zu bytes total", totalBytes);
    }

    closeConnection();

    ESP_LOGI(Tag, "Download complete: %zu bytes", totalBytes);
    return true;
}

bool HttpClient::isStreamOpen() const {
    return (mHttpClient != nullptr);
}

const std::string& HttpClient::getUrl() const {
    return mUrl;
}

int HttpClient::getStatusCode() const {
    if (!mHttpClient) {
        return -1;
    }
    return esp_http_client_get_status_code(mHttpClient);
}

bool HttpClient::openConnection() {
    if (mHttpClient) {
        ESP_LOGW(Tag, "Connection already open");
        return false;
    }

    // Give WiFi driver time to stabilize
    vTaskDelay(pdMS_TO_TICKS(200));

    esp_http_client_config_t cfg = {};
    cfg.url = mUrl.c_str();
    cfg.timeout_ms = mTimeoutMs;
    cfg.buffer_size = mBufferSize;
    cfg.keep_alive_enable = true;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.skip_cert_common_name_check = false;
    cfg.disable_auto_redirect = false;
    cfg.max_redirection_count = 5;

    ESP_LOGI(Tag, "Opening connection: %s", mUrl.c_str());
    mHttpClient = esp_http_client_init(&cfg);
    if (!mHttpClient) {
        ESP_LOGE(Tag, "Failed to initialize HTTP client");
        return false;
    }

    const esp_err_t err = esp_http_client_open(mHttpClient, 0);
    if (err != ESP_OK) {
        ESP_LOGE(Tag, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(mHttpClient);
        mHttpClient = nullptr;
        return false;
    }

    int contentLength = esp_http_client_fetch_headers(mHttpClient);
    ESP_LOGI(Tag, "Content-Length: %d bytes (streaming if 0 or -1)", contentLength);

    const int status = esp_http_client_get_status_code(mHttpClient);
    ESP_LOGI(Tag, "HTTP status: %d", status);
    if (status >= HttpStatusOkThreshold) {
        ESP_LOGE(Tag, "HTTP error status: %d", status);
        closeConnection();
        return false;
    }

    char* headerBuffer = nullptr;
    if (esp_http_client_get_header(mHttpClient, "Transfer-Encoding", &headerBuffer) == ESP_OK) {
        if (headerBuffer) {
            ESP_LOGI(Tag, "Transfer-Encoding: %s", headerBuffer);
        }
    }

    headerBuffer = nullptr;
    if (esp_http_client_get_header(mHttpClient, "Content-Type", &headerBuffer) == ESP_OK) {
        if (headerBuffer) {
            ESP_LOGI(Tag, "Content-Type: %s", headerBuffer);
        }
    }

    // Give server time to start sending audio data
    vTaskDelay(pdMS_TO_TICKS(100));

    return true;
}

void HttpClient::closeConnection() {
    if (mHttpClient) {
        esp_http_client_close(mHttpClient);
        esp_http_client_cleanup(mHttpClient);
        mHttpClient = nullptr;

        ESP_LOGI(Tag, "Connection closed");
    }
}

}  // namespace adapters
