#include "AppControllerTest.hpp"

#include "Events.hpp"

using ::testing::_;

void AppControllerTest::SetUp() {
    mockEventQueue = std::make_unique<common::MockEventQueue>();
    mockPlayerService = std::make_unique<services::MockPlayerService>();
    mockStationRepo = std::make_unique<services::MockStationRepository>();

    appController = std::make_unique<core::AppController>(*mockPlayerService, *mockStationRepo,
                                                          *mockEventQueue);
}

void AppControllerTest::TearDown() {
    appController.reset();

    mockPlayerService.reset();
    mockStationRepo.reset();
    mockEventQueue.reset();
}

TEST_F(AppControllerTest, tc01_init_success) {
    EXPECT_TRUE(appController->init());
}
