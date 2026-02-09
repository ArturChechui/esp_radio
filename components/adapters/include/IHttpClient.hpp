#pragma once

#include <cstdint>
#include <string>

namespace adapters {

class IHttpClient {
   public:
    static constexpr uint32_t DefaultStreamTimeoutMs = 10000U;
    static constexpr uint32_t DefaultDownloadTimeoutMs = 10000U;
    static constexpr size_t DefaultBufferSize = 2048U;

    virtual ~IHttpClient() = default;

    virtual bool openStream(const std::string& url,
                            const uint32_t& timeoutMs = DefaultStreamTimeoutMs) = 0;
    virtual int readStream(uint8_t* buffer, const size_t& size) = 0;
    virtual void closeStream() = 0;
    virtual bool download(const std::string& url, std::string& result,
                          const uint32_t& timeoutMs = DefaultDownloadTimeoutMs) = 0;
    virtual bool isStreamOpen() const = 0;
    virtual const std::string& getUrl() const = 0;
    virtual int getStatusCode() const = 0;
};

}  // namespace adapters
