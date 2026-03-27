#pragma once

#include <gmock/gmock.h>

#include "IPersistentStorage.hpp"

namespace adapters {
class MockPersistentStorage : public IPersistentStorage {
   public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, setString, (const std::string&, const std::string&), (override));
    MOCK_METHOD(bool, getString, (const std::string&, std::string&), (override));
    MOCK_METHOD(bool, setU32, (const std::string&, const uint32_t), (override));
    MOCK_METHOD(bool, getU32, (const std::string&, uint32_t&), (override));
    MOCK_METHOD(bool, erase, (const std::string&), (override));
};
}  // namespace adapters
