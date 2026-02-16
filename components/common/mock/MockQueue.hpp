#pragma once

#include <gmock/gmock.h>

#include "IQueue.hpp"

namespace common {
template <typename T>
class MockQueue : public IQueue<T> {
   public:
    MOCK_METHOD(bool, push, (const T&, const uint32_t), (override));
    MOCK_METHOD(bool, get, (T&), (override));
    MOCK_METHOD(bool, tryGet, (T & out), (override));
};

}  // namespace common
