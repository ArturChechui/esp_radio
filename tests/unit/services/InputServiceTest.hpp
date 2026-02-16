#pragma once

#include <gtest/gtest.h>

#include "InputService.hpp"
#include "MockClock.hpp"
#include "MockEventQueue.hpp"
#include "MockGpioInput.hpp"
#include "MockQueue.hpp"
#include "MockTaskRunner.hpp"
#include "Types.hpp"

using ::testing::StrictMock;

class InputServiceTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<StrictMock<common::MockClock>> mockClock;
    std::unique_ptr<StrictMock<common::MockEventQueue>> mockEventQueue;
    std::unique_ptr<StrictMock<common::MockQueue<uint32_t>>> mockQueue;
    std::unique_ptr<StrictMock<common::MockTaskRunner>> mockTaskRunner;
    std::unique_ptr<StrictMock<adapters::MockGpioInput>> mockGpioInput;
    std::unique_ptr<services::InputService> inputService;

    common::StepFn stepFn = nullptr;
    void* stepUser = nullptr;
};
