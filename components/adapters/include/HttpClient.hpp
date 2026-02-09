#pragma once

#include <esp_http_client.h>

#include <cstdint>
#include <string>

#include "IHttpClient.hpp"

namespace adapters {

class HttpClient : public IHttpClient {
   public:
    HttpClient(const size_t& bufferSize = DefaultBufferSize);
    ~HttpClient() override;

    bool openStream(const std::string& url, const uint32_t& timeoutMs) override;
    int readStream(uint8_t* buffer, const size_t& size) override;
    void closeStream() override;
    bool download(const std::string& url, std::string& result, const uint32_t& timeoutMs) override;
    bool isStreamOpen() const override;
    const std::string& getUrl() const override;
    int getStatusCode() const override;

   private:
    bool openConnection();
    void closeConnection();

    esp_http_client_handle_t mHttpClient;
    std::string mUrl;
    uint32_t mTimeoutMs;
    size_t mBufferSize;
};

}  // namespace adapters
