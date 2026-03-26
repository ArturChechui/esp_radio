#pragma once

#include <gtest/gtest.h>

#include <memory>

#include "MockAdcReader.hpp"
#include "MockClock.hpp"
#include "MockEventQueue.hpp"
#include "MockI2cBus.hpp"
#include "MockTaskRunner.hpp"
#include "SensorService.hpp"
#include "Types.hpp"

using ::testing::NiceMock;
using ::testing::StrictMock;

class SensorServiceTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<StrictMock<adapters::MockI2cBus>> mockI2cBus;
    std::unique_ptr<StrictMock<adapters::MockAdcReader>> mockAdcReader;
    std::unique_ptr<StrictMock<common::MockEventQueue>> mockEventQueue;
    std::unique_ptr<StrictMock<common::MockTaskRunner>> mockTaskRunner;
    std::unique_ptr<NiceMock<common::MockClock>> mockClock;

    std::unique_ptr<services::SensorService> sensorService;

    common::StepFn sensorStepFn = nullptr;
    void* sensorStepUser = nullptr;
    common::StepFn micStepFn = nullptr;
    void* micStepUser = nullptr;
};
