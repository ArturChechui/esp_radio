#pragma once

#include <gmock/gmock.h>

#include "IPlayerService.hpp"

namespace services {
class MockPlayerService : public IPlayerService {
   public:
    MOCK_METHOD(bool, playStation, (const std::string&), (override));
    MOCK_METHOD(bool, stop, (), (override));
    MOCK_METHOD(common::PlaybackStatus, getStatus, (), (const, override));
    MOCK_METHOD(std::string, getCurrentUrl, (), (const, override));
    MOCK_METHOD(int32_t, getVolumeQ15, (), (const, override));
    MOCK_METHOD(void, setVolume, (const uint8_t), (override));
};

}  // namespace services
