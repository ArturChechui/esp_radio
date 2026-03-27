#pragma once

#include <gtest/gtest.h>

#include <memory>

#include "AppController.hpp"
#include "JsonParser.hpp"
#include "MockEventQueue.hpp"
#include "MockFileSystem.hpp"
#include "MockHttpClient.hpp"
#include "MockInputService.hpp"
#include "MockJsonParser.hpp"
#include "MockPlayerService.hpp"
#include "MockSensorService.hpp"
#include "MockStationRepository.hpp"
#include "MockWifiService.hpp"

class AppControllerTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<services::MockWifiService> mockWifiService;
    std::unique_ptr<common::MockEventQueue> mockEventQueue;
    std::unique_ptr<services::MockPlayerService> mockPlayerService;
    std::unique_ptr<services::MockStationRepository> mockStationRepo;
    std::unique_ptr<services::MockSensorService> mockSensorService;
    std::unique_ptr<services::MockInputService> mockInputService;
    std::unique_ptr<adapters::MockHttpClient> mockHttpClient;
    std::unique_ptr<adapters::MockFileSystem> mockFileSystem;
    std::unique_ptr<common::MockJsonParser> mockJsonParser;

    std::unique_ptr<core::AppController> appController;
};
