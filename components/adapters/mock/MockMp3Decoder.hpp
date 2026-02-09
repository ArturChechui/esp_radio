#pragma once

#include <gmock/gmock.h>

#include "IMp3Decoder.hpp"

namespace adapters {
class MockMp3Decoder : public IMp3Decoder {
   public:
    MOCK_METHOD(common::Mp3FrameInfo, decode, (const uint8_t*, size_t, int16_t*, size_t),
                (override));
    MOCK_METHOD(void, reset, (), (override));
};

}  // namespace adapters
