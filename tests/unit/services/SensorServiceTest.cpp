#include "SensorServiceTest.hpp"

#include <array>
#include <cstdint>
#include <cstring>

#include "BoardConfig.hpp"
#include "Events.hpp"
#include "Fonts.hpp"
#include "MockStopToken.hpp"
#include "Types.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {}  // namespace

void SensorServiceTest::SetUp() {
    mockEventQueue = std::make_unique<StrictMock<common::MockEventQueue>>();
    mockTaskRunner = std::make_unique<StrictMock<common::MockTaskRunner>>();
    mockI2cBus = std::make_unique<StrictMock<adapters::MockI2cBus>>();

    sensorService =
        std::make_unique<services::SensorService>(*mockI2cBus, *mockEventQueue, *mockTaskRunner);
}

void SensorServiceTest::TearDown() {
    sensorService.reset();

    mockEventQueue.reset();
    mockTaskRunner.reset();
    mockI2cBus.reset();
}

TEST_F(SensorServiceTest, tc01_init_success) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    ASSERT_NE(stepFn, nullptr);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc02_init_deinit_success) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    EXPECT_TRUE(sensorService->init());
    ASSERT_NE(stepFn, nullptr);

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    sensorService->deinit();
}

TEST_F(SensorServiceTest, tc03_init_fail) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _)).WillOnce(Return(common::TaskHandle{}));

    EXPECT_FALSE(sensorService->init());
}

TEST_F(SensorServiceTest, tc04_readStep_success) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockI2cBus, readBytes(_, _, _, _))
        .WillRepeatedly([](const uint8_t deviceAddr, uint8_t* data, const size_t len,
                           const uint32_t timeoutMs) {
            std::array<uint8_t, 6> res = {0x08, 0x8C, 0xCC, 0xC5, 0xEB, 0x85};
            std::memcpy(data, res.data(), len);
            return true;
        });
    EXPECT_CALL(token, sleepMs(_)).Times(3).WillRepeatedly(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        bool res = false;
        if (const auto* e = std::get_if<common::TempHumidUpdateEvent>(&event)) {
            EXPECT_EQ(e->temperature, 24);
            EXPECT_EQ(e->humidity, 55);
            res = true;
        }
        EXPECT_TRUE(res);
        return res;
    });

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc05_readStep_maxData) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockI2cBus, readBytes(_, _, _, _))
        .WillRepeatedly([](const uint8_t deviceAddr, uint8_t* data, const size_t len,
                           const uint32_t timeoutMs) {
            std::array<uint8_t, 6> res = {0x08, 0xFF, 0xFF, 0xFC, 0x00, 0x00};

            std::memcpy(data, res.data(), len);
            return true;
        });
    EXPECT_CALL(token, sleepMs(_)).Times(3).WillRepeatedly(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        bool res = false;
        if (const auto* e = std::get_if<common::TempHumidUpdateEvent>(&event)) {
            EXPECT_EQ(e->temperature, 99);
            EXPECT_EQ(e->humidity, 99);
            res = true;
        }
        EXPECT_TRUE(res);
        return res;
    });

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc06_readStep_minData) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockI2cBus, readBytes(_, _, _, _))
        .WillRepeatedly([](const uint8_t deviceAddr, uint8_t* data, const size_t len,
                           const uint32_t timeoutMs) {
            std::array<uint8_t, 6> res = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00};

            std::memcpy(data, res.data(), len);
            return true;
        });
    EXPECT_CALL(token, sleepMs(_)).Times(3).WillRepeatedly(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        bool res = false;
        if (const auto* e = std::get_if<common::TempHumidUpdateEvent>(&event)) {
            EXPECT_EQ(e->temperature, -50);
            EXPECT_EQ(e->humidity, 0);
            res = true;
        }
        EXPECT_TRUE(res);
        return res;
    });

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc07_readStep_notCalibrated) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockI2cBus, readBytes(_, _, _, _))
        .WillRepeatedly([](const uint8_t deviceAddr, uint8_t* data, const size_t len,
                           const uint32_t timeoutMs) {
            std::array<uint8_t, 6> res = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            std::memcpy(data, res.data(), len);
            return true;
        });
    EXPECT_CALL(token, sleepMs(_)).Times(3).WillRepeatedly(Return(false));

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc08_readStep_busy) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockI2cBus, readBytes(_, _, _, _))
        .WillRepeatedly([](const uint8_t deviceAddr, uint8_t* data, const size_t len,
                           const uint32_t timeoutMs) {
            std::array<uint8_t, 6> res = {0x88, 0x00, 0x00, 0x00, 0x00, 0x00};

            std::memcpy(data, res.data(), len);
            return true;
        });
    EXPECT_CALL(token, sleepMs(_)).Times(3).WillRepeatedly(Return(false));

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc09_readStep_aht20ResetFail) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).WillOnce(Return(false));

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc10_readStep_aht20InitFail) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).WillOnce(Return(true)).WillOnce(Return(false));

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc11_readStep_aht20TriggerFail) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}

TEST_F(SensorServiceTest, tc12_readStep_aht20ReadFail) {
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    sensorService->init();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockI2cBus, readBytes(_, _, _, _)).WillOnce(Return(false));

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    // Destructor
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
}
