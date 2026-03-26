#pragma once

#include <gtest/gtest.h>

#include "MockDisplay.hpp"
#include "MockStationRepository.hpp"
#include "UiService.hpp"

using ::testing::StrictMock;

class UiServiceTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;
    void initSuccess();
    void switchToMainScreenSuccess();
    void switchToWifiProvScreenSuccess();
    void switchToSyncInProgressScreenSuccess();

    std::unique_ptr<StrictMock<adapters::MockDisplay>> mockDisplay;
    std::unique_ptr<StrictMock<services::MockStationRepository>> mockRepo;
    std::unique_ptr<services::UiService> uiService;
};
