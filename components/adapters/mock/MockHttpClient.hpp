#pragma once

#include <gmock/gmock.h>

#include "IHttpClient.hpp"

namespace adapters {
class MockHttpClient : public IHttpClient {
   public:
    MOCK_METHOD(bool, openStream, (const std::string&, const uint32_t&), (override));
    MOCK_METHOD(int, readStream, (uint8_t*, const size_t&), (override));

    MOCK_METHOD(void, closeStream, (), (override));
    MOCK_METHOD(bool, download, (const std::string&, std::string&, const uint32_t&), (override));
    MOCK_METHOD(bool, isStreamOpen, (), (const, override));
    MOCK_METHOD(const std::string&, getUrl, (), (const, override));
    MOCK_METHOD(int, getStatusCode, (), (const, override));
};

}  // namespace adapters
