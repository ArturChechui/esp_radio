#include "InputServiceTest.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "BoardConfig.hpp"
#include "Events.hpp"
#include "Fonts.hpp"
#include "MockStopToken.hpp"
#include "Types.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {}  // namespace

void InputServiceTest::SetUp() {
    mockClock = std::make_unique<StrictMock<common::MockClock>>();
    mockEventQueue = std::make_unique<StrictMock<common::MockEventQueue>>();
    mockQueue = std::make_unique<StrictMock<common::MockQueue<uint32_t>>>();
    mockTaskRunner = std::make_unique<StrictMock<common::MockTaskRunner>>();
    mockGpioInput = std::make_unique<StrictMock<adapters::MockGpioInput>>();
    mockPersistentStorage = std::make_unique<StrictMock<adapters::MockPersistentStorage>>();

    inputService = std::make_unique<services::InputService>(*mockGpioInput, *mockEventQueue,
                                                            *mockQueue, *mockTaskRunner, *mockClock,
                                                            *mockPersistentStorage);
}

void InputServiceTest::TearDown() {
    inputService.reset();

    mockClock.reset();
    mockEventQueue.reset();
    mockQueue.reset();
    mockTaskRunner.reset();
    mockGpioInput.reset();
}

void InputServiceTest::initSuccess() {
    EXPECT_CALL(*mockPersistentStorage, getU32("volume", _)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            stepFn = fn;
            stepUser = user;
            return common::TaskHandle{0, 1};
        });

    inputService->init();

    ASSERT_NE(stepFn, nullptr);
}

TEST_F(InputServiceTest, tc01_init_success) {
    initSuccess();
}

TEST_F(InputServiceTest, tc02_stepFn_playStop) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillOnce([](uint32_t& out) {
        out = common::ButtonPlayStopGpio;
        return true;
    });
    EXPECT_CALL(*mockClock, nowMs())
        .WillOnce(Return(300))
        .WillOnce(Return(325))
        .WillOnce(Return(330));
    EXPECT_CALL(*mockGpioInput, getLevel(common::ButtonPlayStopGpio))
        .WillOnce(Return(0))
        .WillOnce(Return(0))
        .WillOnce(Return(1));
    EXPECT_CALL(token, sleepMs(_)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::ButtonPressedEvent>(&event)) {
            EXPECT_EQ(e->button, common::Button::PlayStop);
            return true;
        }

        return false;
    });

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);
}

TEST_F(InputServiceTest, tc09_stepFn_playStop_longPress) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillOnce([](uint32_t& out) {
        out = common::ButtonPlayStopGpio;
        return true;
    });
    EXPECT_CALL(*mockClock, nowMs())
        .WillOnce(Return(300))
        .WillOnce(Return(325))
        .WillOnce(Return(3325))
        .WillOnce(Return(3330));
    EXPECT_CALL(*mockGpioInput, getLevel(common::ButtonPlayStopGpio))
        .WillOnce(Return(0))
        .WillOnce(Return(0))
        .WillOnce(Return(0))
        .WillOnce(Return(0));
    EXPECT_CALL(token, sleepMs(_)).WillOnce(Return(false)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::ButtonLongPressedEvent>(&event)) {
            EXPECT_EQ(e->button, common::Button::PlayStop);
            return true;
        }

        return false;
    });

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);
}

TEST_F(InputServiceTest, tc03_stepFn_next) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillOnce([](uint32_t& out) {
        out = common::ButtonNextGpio;
        return true;
    });
    EXPECT_CALL(*mockClock, nowMs()).WillOnce(Return(300)).WillOnce(Return(325));
    EXPECT_CALL(*mockGpioInput, getLevel(common::ButtonNextGpio))
        .Times(2)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(token, sleepMs(_)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::ButtonPressedEvent>(&event)) {
            EXPECT_EQ(e->button, common::Button::Next);
            return true;
        }

        return false;
    });

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);
}

TEST_F(InputServiceTest, tc04_stepFn_prev) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillOnce([](uint32_t& out) {
        out = common::ButtonPrevGpio;
        return true;
    });
    EXPECT_CALL(*mockClock, nowMs()).WillOnce(Return(300)).WillOnce(Return(325));
    EXPECT_CALL(*mockGpioInput, getLevel(common::ButtonPrevGpio))
        .Times(2)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(token, sleepMs(_)).WillOnce(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::ButtonPressedEvent>(&event)) {
            EXPECT_EQ(e->button, common::Button::Previous);
            return true;
        }

        return false;
    });

    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);
}

TEST_F(InputServiceTest, tc05_stepFn_enc_increaseVol) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillRepeatedly([](uint32_t& out) {
        out = common::EncS1Gpio;
        return true;
    });

    EXPECT_CALL(*mockGpioInput, getLevel(_)).WillOnce(Return(0)).WillOnce(Return(0));
    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    EXPECT_CALL(*mockPersistentStorage, setU32("volume", 13)).WillOnce(Return(true));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        bool res = false;
        if (const auto* e = std::get_if<common::VolumeChangedEvent>(&event)) {
            EXPECT_EQ(e->volume, 13);
            res = true;
        }
        return res;
    });

    // 4 calls to make a single step to increase vol level.
    // Transitions:
    // 00 -> 10
    // 10 -> 11
    // 11 -> 01
    // 01 -> 00
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS1Gpio))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(0))
        .WillOnce(Return(0));
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS2Gpio))
        .WillOnce(Return(0))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    for (int i = 0; i < 4; i++) {
        stepFn(stepUser, token);
    }
}

TEST_F(InputServiceTest, tc06_stepFn_enc_decreaseVol) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillRepeatedly([](uint32_t& out) {
        out = common::EncS1Gpio;
        return true;
    });

    EXPECT_CALL(*mockGpioInput, getLevel(_)).WillOnce(Return(0)).WillOnce(Return(0));
    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    EXPECT_CALL(*mockPersistentStorage, setU32("volume", 7)).WillOnce(Return(true));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        bool res = false;
        if (const auto* e = std::get_if<common::VolumeChangedEvent>(&event)) {
            EXPECT_EQ(e->volume, 7);
            res = true;
        }
        return res;
    });

    // 4 calls to make a single step to decrease vol level.
    // Transitions:
    // 00 -> 01
    // 01 -> 11
    // 11 -> 10
    // 10 -> 00
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS1Gpio))
        .WillOnce(Return(0))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS2Gpio))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(0))
        .WillOnce(Return(0));
    for (int i = 0; i < 4; i++) {
        stepFn(stepUser, token);
    }
}

TEST_F(InputServiceTest, tc07_stepFn_enc_3qStepsDec_noPost) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillRepeatedly([](uint32_t& out) {
        out = common::EncS1Gpio;
        return true;
    });

    EXPECT_CALL(*mockGpioInput, getLevel(_)).WillOnce(Return(0)).WillOnce(Return(0));
    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    EXPECT_CALL(*mockEventQueue, post(_)).Times(0);
    // 3 calls to make 3 quarter steps to dec, no full step.
    // Transitions:
    // 00 -> 01
    // 01 -> 11
    // 11 -> 10
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS1Gpio))
        .WillOnce(Return(0))
        .WillOnce(Return(1))
        .WillOnce(Return(1));
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS2Gpio))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    for (int i = 0; i < 3; i++) {
        stepFn(stepUser, token);
    }
}

TEST_F(InputServiceTest, tc08_stepFn_enc_3qStepsInc_noPost) {
    initSuccess();

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockQueue, get(_)).WillRepeatedly([](uint32_t& out) {
        out = common::EncS1Gpio;
        return true;
    });

    EXPECT_CALL(*mockGpioInput, getLevel(_)).WillOnce(Return(0)).WillOnce(Return(0));
    ASSERT_NE(stepFn, nullptr);
    stepFn(stepUser, token);

    EXPECT_CALL(*mockEventQueue, post(_)).Times(0);
    // 3 calls to make 3 quarter steps to inc, no full step.
    // Transitions:
    // 00 -> 10
    // 10 -> 11
    // 11 -> 01
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS1Gpio))
        .WillOnce(Return(1))
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    EXPECT_CALL(*mockGpioInput, getLevel(common::EncS2Gpio))
        .WillOnce(Return(0))
        .WillOnce(Return(1))
        .WillOnce(Return(1));
    for (int i = 0; i < 3; i++) {
        stepFn(stepUser, token);
    }
}

TEST_F(InputServiceTest, tc10_init_deinit_success) {
    initSuccess();

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    inputService->deinit();
}
