#pragma once

#include <gtest/gtest.h>

#include <memory>

#include "MockEventQueue.hpp"
#include "MockI2cBus.hpp"
#include "MockTaskRunner.hpp"
#include "SensorService.hpp"
#include "Types.hpp"

using ::testing::StrictMock;

class SensorServiceTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<StrictMock<adapters::MockI2cBus>> mockI2cBus;
    std::unique_ptr<StrictMock<common::MockEventQueue>> mockEventQueue;
    std::unique_ptr<StrictMock<common::MockTaskRunner>> mockTaskRunner;

    std::unique_ptr<services::SensorService> sensorService;

    common::StepFn stepFn = nullptr;
    void* stepUser = nullptr;
};
