#pragma once

#include <gtest/gtest.h>

#include <memory>

#include "AppController.hpp"
#include "MockEventQueue.hpp"
#include "MockPlayerService.hpp"
#include "MockStationRepository.hpp"

class AppControllerTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<common::MockEventQueue> mockEventQueue;
    std::unique_ptr<services::MockPlayerService> mockPlayerService;
    std::unique_ptr<services::MockStationRepository> mockStationRepo;

    std::unique_ptr<core::AppController> appController;
};
