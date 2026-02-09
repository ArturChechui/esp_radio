#pragma once

#include <gmock/gmock.h>

#include "IPlayerService.hpp"

namespace services {
class MockPlayerService : public IPlayerService {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, playStation, (const std::string&), (override));
    MOCK_METHOD(bool, stop, (), (override));
    MOCK_METHOD(common::PlaybackStatus, getStatus, (), (const, override));
    MOCK_METHOD(std::string, getCurrentUrl, (), (const, override));
};

}  // namespace services
