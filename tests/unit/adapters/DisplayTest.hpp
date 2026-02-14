#pragma once

#include <gtest/gtest.h>

#include "Display.hpp"
#include "MockI2cBus.hpp"

class DisplayTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;

    void initOkExpectations();

    adapters::MockI2cBus mockI2cBus;
    std::unique_ptr<adapters::Display> display;
};
