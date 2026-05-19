#include "SensorServiceTest.hpp"

#include <array>
#include <cstdint>
#include <cstring>

#include "BoardConfig.hpp"
#include "Events.hpp"
#include "MockStopToken.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;

namespace {
void fillQuiet(int* buffer, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
        buffer[i] = 2000 + static_cast<int>(i % 7U) - 3;
    }
}

void fillWeakButFast(int* buffer, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
        buffer[i] = (i % 2U == 0U) ? 1800 : 2200;  // p2p=400, diff=400
    }
}

void fillLoudClapLike(int* buffer, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
        buffer[i] = (i % 2U == 0U) ? 1200 : 3000;  // strong transient
    }
}
}  // namespace

void SensorServiceTest::SetUp() {
    mockEventQueue = std::make_unique<StrictMock<common::MockEventQueue>>();
    mockTaskRunner = std::make_unique<StrictMock<common::MockTaskRunner>>();
    mockI2cBus = std::make_unique<StrictMock<adapters::MockI2cBus>>();
    mockAdcReader = std::make_unique<StrictMock<adapters::MockAdcReader>>();
    mockClock = std::make_unique<NiceMock<common::MockClock>>();
    mockPersistentStorage = std::make_unique<StrictMock<adapters::MockPersistentStorage>>();

    EXPECT_CALL(*mockClock, nowMs()).Times(AnyNumber()).WillRepeatedly(Return(0U));

    sensorService = std::make_unique<services::SensorService>(*mockI2cBus, *mockAdcReader,
                                                              *mockEventQueue, *mockTaskRunner,
                                                              *mockClock, *mockPersistentStorage);
}

void SensorServiceTest::TearDown() {
    sensorService.reset();
    mockClock.reset();
    mockEventQueue.reset();
    mockTaskRunner.reset();
    mockI2cBus.reset();
    mockAdcReader.reset();
}

TEST_F(SensorServiceTest, tc01_init_success) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 1;
            return true;
        });

    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));

    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams& params, uint32_t, common::StepFn fn, void* user) {
            EXPECT_STREQ(params.name, "SensorTask");
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce([&](const common::TaskParams& params, uint32_t, common::StepFn fn, void* user) {
            EXPECT_STREQ(params.name, "MicTask");
            micStepFn = fn;
            micStepUser = user;
            return common::TaskHandle{1, 1};
        });

    ASSERT_TRUE(sensorService->init());
    ASSERT_NE(sensorStepFn, nullptr);
    ASSERT_NE(micStepFn, nullptr);

    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(2)
        .WillRepeatedly(Return(common::StopResult::Ok));
    sensorService->deinit();
}

TEST_F(SensorServiceTest, tc02_init_setupChannelMicFail) {
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(false));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).Times(0);
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _)).Times(0);

    EXPECT_FALSE(sensorService->init());
}

TEST_F(SensorServiceTest, tc03_init_setupChannelBatFail) {
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(false));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _)).Times(0);

    EXPECT_FALSE(sensorService->init());
}

TEST_F(SensorServiceTest, tc04_init_micTaskFail) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 1;
            return true;
        });
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce(Return(common::TaskHandle{}));
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));

    EXPECT_FALSE(sensorService->init());
}

TEST_F(SensorServiceTest, tc05_sensorStepFn_success) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 1;
            return true;
        });
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            micStepFn = fn;
            micStepUser = user;
            return common::TaskHandle{1, 1};
        });
    ASSERT_TRUE(sensorService->init());
    ASSERT_NE(sensorStepFn, nullptr);

    EXPECT_CALL(*mockI2cBus, writeBytes(_, _, _, _)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockI2cBus, readBytes(_, _, _, _))
        .Times(2)
        .WillOnce(Invoke([](uint8_t, uint8_t* data, size_t len, uint32_t) {
            const std::array<uint8_t, 6> sample = {0x08, 0x8C, 0xCC, 0xC5, 0xEB, 0x85};
            std::memcpy(data, sample.data(), len);
            return true;
        }))
        .WillOnce(Invoke([](uint8_t, uint8_t* data, size_t len, uint32_t) {
            const std::array<uint8_t, 2> sample = {0x01, 0x2C};  // raw=300 => lux=250
            std::memcpy(data, sample.data(), len);
            return true;
        }));

    EXPECT_CALL(*mockAdcReader, readRawBurst(common::BatteryAdcGpio, _, 5))
        .WillOnce(Invoke([](uint32_t, int* buffer, size_t count) {
            for (size_t i = 0; i < count; ++i) {
                buffer[i] = 2605;
            }
            return true;
        }));
    EXPECT_CALL(*mockAdcReader, readRawBurst(common::MicAdcGpio, _, _)).Times(0);

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(token, sleepMs(_)).WillRepeatedly(Return(false));

    InSequence seq;
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Invoke([](const common::AppEvent& e) {
        const auto* temp = std::get_if<common::TempHumidUpdateEvent>(&e);
        EXPECT_NE(temp, nullptr);
        EXPECT_EQ(temp->temperature, 24);
        EXPECT_EQ(temp->humidity, 55);
        return true;
    }));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Invoke([](const common::AppEvent& e) {
        const auto* light = std::get_if<common::LightLevelUpdateEvent>(&e);
        EXPECT_NE(light, nullptr);
        EXPECT_EQ(light->lux, 250);
        return true;
    }));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Invoke([](const common::AppEvent& e) {
        const auto* batt = std::get_if<common::BatteryLevelUpdateEvent>(&e);
        EXPECT_NE(batt, nullptr);
        EXPECT_EQ(batt->millivolts, 4198);
        EXPECT_EQ(batt->percent, 99);
        return true;
    }));

    const common::StepResult r = sensorStepFn(sensorStepUser, token);
    EXPECT_EQ(r.action, common::StepAction::Sleep);
    EXPECT_EQ(r.sleepMs, 30000U);

    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(2)
        .WillRepeatedly(Return(common::StopResult::Ok));
    sensorService->deinit();
}

TEST_F(SensorServiceTest, tc06_listenStepFn_success) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 1;
            return true;
        });
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            micStepFn = fn;
            micStepUser = user;
            return common::TaskHandle{1, 1};
        });
    ASSERT_TRUE(sensorService->init());
    ASSERT_NE(micStepFn, nullptr);

    EXPECT_CALL(*mockAdcReader, readRawBurst(common::MicAdcGpio, _, 96))
        .WillOnce(Invoke([](uint32_t, int* buffer, size_t count) {
            fillQuiet(buffer, count);
            return true;
        }))
        .WillOnce(Invoke([](uint32_t, int* buffer, size_t count) {
            fillLoudClapLike(buffer, count);
            return true;
        }));

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).Times(2).WillRepeatedly(Return(false));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::ButtonPressedEvent>(&event)) {
            EXPECT_EQ(e->button, common::Button::PlayStop);
            return true;
        }

        return false;
    });

    const common::StepResult r1 = micStepFn(micStepUser, token);
    const common::StepResult r2 = micStepFn(micStepUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Sleep);
    EXPECT_EQ(r1.sleepMs, 5U);
    EXPECT_EQ(r2.action, common::StepAction::Sleep);
    EXPECT_EQ(r2.sleepMs, 3000U);

    // Playback started -> stop clap
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    sensorService->startClapDetection(false);

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    sensorService->deinit();
}

TEST_F(SensorServiceTest, tc07_listenStepFn_weakFast_loud) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 1;
            return true;
        });
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            micStepFn = fn;
            micStepUser = user;
            return common::TaskHandle{1, 1};
        });
    ASSERT_TRUE(sensorService->init());
    ASSERT_NE(micStepFn, nullptr);

    EXPECT_CALL(*mockAdcReader, readRawBurst(common::MicAdcGpio, _, 96))
        .WillOnce(Invoke([](uint32_t, int* buffer, size_t count) {
            fillQuiet(buffer, count);
            return true;
        }))
        .WillOnce(Invoke([](uint32_t, int* buffer, size_t count) {
            fillWeakButFast(buffer, count);  // high diff but p2p too low
            return true;
        }))
        .WillOnce(Invoke([](uint32_t, int* buffer, size_t count) {
            fillLoudClapLike(buffer, count);  // passes all gates
            return true;
        }));

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce([](const common::AppEvent& event) {
        if (const auto* e = std::get_if<common::ButtonPressedEvent>(&event)) {
            EXPECT_EQ(e->button, common::Button::PlayStop);
            return true;
        }

        return false;
    });

    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).Times(3).WillRepeatedly(Return(false));

    (void)micStepFn(micStepUser, token);
    (void)micStepFn(micStepUser, token);
    (void)micStepFn(micStepUser, token);

    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(2)
        .WillRepeatedly(Return(common::StopResult::Ok));
    sensorService->deinit();
}

TEST_F(SensorServiceTest, tc08_listenStepFn_playing) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 1;
            return true;
        });
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            micStepFn = fn;
            micStepUser = user;
            return common::TaskHandle{1, 1};
        });
    ASSERT_TRUE(sensorService->init());
    ASSERT_NE(micStepFn, nullptr);

    // Playback started - need to stop Clap Detection
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    sensorService->startClapDetection(false);

    // Should be no calls towards adc reader
    EXPECT_CALL(*mockAdcReader, readRawBurst(common::MicAdcGpio, _, _)).Times(0);
    common::MockStopToken token;
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(true));
    const common::StepResult r = micStepFn(micStepUser, token);
    EXPECT_EQ(r.action, common::StepAction::Done);

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    sensorService->deinit();
}

TEST_F(SensorServiceTest, tc09_startClapDetection_сlapFeatureDisabled) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 0;
            return true;
        });
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        });
    ASSERT_TRUE(sensorService->init());

    // no calls, hard exit
    sensorService->startClapDetection(true);

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    sensorService->deinit();
}

TEST_F(SensorServiceTest, tc10_toggleClapFeature_disable_enable_stoppedPlayback) {
    EXPECT_CALL(*mockPersistentStorage, getU32("micfeature", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 1;
            return true;
        });
    EXPECT_CALL(*mockAdcReader, setupChannel(common::MicAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockAdcReader, setupChannel(common::BatteryAdcGpio)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            sensorStepFn = fn;
            sensorStepUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            micStepFn = fn;
            micStepUser = user;
            return common::TaskHandle{1, 1};
        });
    ASSERT_TRUE(sensorService->init());
    ASSERT_NE(micStepFn, nullptr);

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    EXPECT_CALL(*mockPersistentStorage, setU32("micfeature", 0)).WillOnce(Return(true));

    EXPECT_FALSE(sensorService->toggleClapFeature());

    // playback stopped
    sensorService->startClapDetection(true);
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            micStepFn = fn;
            micStepUser = user;
            return common::TaskHandle{1, 1};
        });
    EXPECT_CALL(*mockPersistentStorage, setU32("micfeature", 1)).WillOnce(Return(true));
    EXPECT_TRUE(sensorService->toggleClapFeature());

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    EXPECT_CALL(*mockPersistentStorage, setU32("micfeature", 0)).WillOnce(Return(true));
    EXPECT_FALSE(sensorService->toggleClapFeature());

    common::MockStopToken token;
    const common::StepResult r = micStepFn(micStepUser, token);
    EXPECT_EQ(r.action, common::StepAction::Done);

    // no calls, hard exit
    sensorService->startClapDetection(true);

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    sensorService->deinit();
}
