#include "PlayerServiceTest.hpp"

#include "FakeRingBuffer.hpp"
#include "FakeSignal.hpp"
#include "MockStopToken.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

void PlayerServiceTest::SetUp() {
    mockHttpClient = std::make_unique<adapters::MockHttpClient>();
    mockI2sBus = std::make_unique<adapters::MockI2sBus>();
    mockMp3Decoder = std::make_unique<adapters::MockMp3Decoder>();

    fakeStats = std::make_unique<common::FakeAudioBufferStats>(10000);
    mockEventQueue = std::make_unique<common::MockEventQueue>();
    mockTaskRunner = std::make_unique<common::MockTaskRunner>();

    auto rb = std::make_unique<common::FakeRingBuffer>(services::PlayerService::RingBufferSize);
    fakeRing = rb.get();

    playerService = std::make_unique<services::PlayerService>(
        *mockI2sBus, *mockHttpClient, *mockMp3Decoder, *mockTaskRunner, std::move(rb), *fakeStats,
        *mockEventQueue, std::make_unique<common::FakeSignal>());
}

void PlayerServiceTest::TearDown() {
    playerService.reset();

    mockHttpClient.reset();
    mockI2sBus.reset();
    mockMp3Decoder.reset();
    fakeStats.reset();
    mockEventQueue.reset();
    mockTaskRunner.reset();

    fakeRing = nullptr;
}

void PlayerServiceTest::expectStartCaptureBothStepFns() {
    // Capture StepFn+user for HttpTask (1st start) and PlayerTask (2nd start)
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            httpFn = fn;
            httpUser = user;
            return common::TaskHandle{0, 1};
        })
        .WillOnce([&](const common::TaskParams&, uint32_t, common::StepFn fn, void* user) {
            playerFn = fn;
            playerUser = user;
            return common::TaskHandle{1, 1};
        });
}

void PlayerServiceTest::configureNonStoppingToken(common::MockStopToken& token) {
    ON_CALL(token, stopRequested()).WillByDefault(Return(false));
    ON_CALL(token, sleepMs(_)).WillByDefault(Return(false));
}

TEST_F(PlayerServiceTest, tc01_init_success) {
    EXPECT_TRUE(true);
}

TEST_F(PlayerServiceTest, tc02_playStation_emptyUrl) {
    EXPECT_FALSE(playerService->playStation(""));
    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Idle);
}

TEST_F(PlayerServiceTest, tc03_playStation_noStepStart_success) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    const std::string url = "http://example.com/stream.mp3";
    EXPECT_TRUE(playerService->playStation(url));

    ASSERT_NE(httpFn, nullptr);
    ASSERT_NE(playerFn, nullptr);
    ASSERT_NE(httpUser, nullptr);
    ASSERT_NE(playerUser, nullptr);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Buffering);
    EXPECT_EQ(playerService->getCurrentUrl(), url);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc04_playStation_playerTaskFail) {
    InSequence seq;

    const common::TaskHandle httpHandle{0, 1};
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _)).WillOnce(Return(httpHandle));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _)).WillOnce(Return(common::TaskHandle{}));

    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));

    EXPECT_FALSE(playerService->playStation("http://example.com/stream.mp3"));
    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Error);
}

TEST_F(PlayerServiceTest, tc05_playStation_openStreamFail) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://bad.url/stream.mp3"));

    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(false));

    common::MockStopToken token;
    configureNonStoppingToken(token);

    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    ASSERT_NE(httpFn, nullptr);
    const common::StepResult r = httpFn(httpUser, token);
    EXPECT_EQ(r.action, common::StepAction::Sleep);
    EXPECT_EQ(r.sleepMs, 500U);
    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Buffering);

    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    ASSERT_NE(playerFn, nullptr);
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Sleep);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc06_playStation_streamNotOpen_continue) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    common::MockStopToken token;
    configureNonStoppingToken(token);
    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    const common::StepResult r = playerFn(playerUser, token);
    EXPECT_EQ(r.action, common::StepAction::Sleep);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc07_playStation_notPrebuffered_sleep) {
    expectStartCaptureBothStepFns();

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));

    EXPECT_CALL(*mockHttpClient, readStream(_, _)).WillOnce(Return(0));
    common::MockStopToken token;
    configureNonStoppingToken(token);
    EXPECT_CALL(token, stopRequested()).Times(2).WillRepeatedly(Return(false));
    ASSERT_NE(httpFn, nullptr);
    const common::StepResult r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Sleep);
    EXPECT_EQ(r1.sleepMs, 300U);

    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Sleep);
    EXPECT_EQ(r2.sleepMs, 20U);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc08_playStation_decodeFrame0_drop1Byte) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    // simulate that http task worked
    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, readStream(_, _)).WillOnce(Return(0));
    common::MockStopToken token;
    configureNonStoppingToken(token);
    ASSERT_NE(httpFn, nullptr);
    EXPECT_CALL(token, stopRequested()).Times(2).WillRepeatedly(Return(false));
    const common::StepResult r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Sleep);
    EXPECT_EQ(r1.sleepMs, 300U);

    // fill RB to 90KB for buffer condition
    std::vector<uint8_t> data(90U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data.data(), data.size()), data.size());
    const size_t before = fakeRing->available();
    ASSERT_EQ(before, 92160U);

    common::Mp3FrameInfo info{};
    info.frameBytes = 0;
    EXPECT_CALL(*mockMp3Decoder, decode(_, _, _, _)).WillOnce(Return(info));

    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Continue);

    const size_t after = fakeRing->available();
    EXPECT_EQ(after, before - 1U);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Buffering);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc09_playStation_fullPath_stereo_success) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    // Open stream once and read 4 times
    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, readStream(_, _))
        .Times(4)
        .WillRepeatedly([](uint8_t* dst, const size_t& len) -> int {
            std::memset(dst, 0xAA, len);
            return static_cast<int>(len);
        });
    common::MockStopToken token;
    configureNonStoppingToken(token);
    ASSERT_NE(httpFn, nullptr);
    // 4 http steps
    EXPECT_CALL(token, stopRequested()).Times(8).WillRepeatedly(Return(false));
    common::StepResult r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);

    // Fill RB to make the buffer condition pass
    std::vector<uint8_t> data1(112U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data1.data(), data1.size()), data1.size() - 1);

    common::Mp3FrameInfo info{
        .frameBytes = adapters::MaxBytesPerFrame, .hz = 44100, .channels = 2, .samplesPerCh = 1152};
    EXPECT_CALL(*mockMp3Decoder, decode(_, _, _, _)).WillOnce(Return(info));
    EXPECT_CALL(*mockI2sBus, getSampleRate()).WillOnce(Return(44100));
    EXPECT_CALL(*mockI2sBus, write(_, 2304, 600)).Times(2).WillRepeatedly(Return(2304));
    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Continue);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Playing);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc10_playStation_fullPath_mono_success) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    // Open stream once and read 4 times
    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, readStream(_, _))
        .Times(4)
        .WillRepeatedly([](uint8_t* dst, const size_t& len) -> int {
            std::memset(dst, 0xAA, len);
            return static_cast<int>(len);
        });
    common::MockStopToken token;
    configureNonStoppingToken(token);
    ASSERT_NE(httpFn, nullptr);
    // 4 http steps
    EXPECT_CALL(token, stopRequested()).Times(8).WillRepeatedly(Return(false));
    common::StepResult r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);

    // Fill RB to make the buffer condition pass
    std::vector<uint8_t> data1(112U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data1.data(), data1.size()), data1.size() - 1);

    common::Mp3FrameInfo info{
        .frameBytes = adapters::MaxBytesPerFrame, .hz = 44100, .channels = 1, .samplesPerCh = 1152};
    EXPECT_CALL(*mockMp3Decoder, decode(_, _, _, _)).WillOnce(Return(info));
    EXPECT_CALL(*mockI2sBus, getSampleRate()).WillOnce(Return(44100));
    EXPECT_CALL(*mockI2sBus, write(_, 2304, 600)).Times(2).WillRepeatedly(Return(2304));
    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Continue);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Playing);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc11_playStation_stop) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    const std::string url = "http://example.com/stream.mp3";
    EXPECT_TRUE(playerService->playStation(url));

    ASSERT_NE(httpFn, nullptr);
    ASSERT_NE(playerFn, nullptr);
    ASSERT_NE(httpUser, nullptr);
    ASSERT_NE(playerUser, nullptr);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Buffering);
    EXPECT_EQ(playerService->getCurrentUrl(), url);

    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_TRUE(playerService->stop());

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Stopped);
    EXPECT_EQ(playerService->getCurrentUrl(), "");
}

TEST_F(PlayerServiceTest, tc12_playStation_decodeFail_retry_decodeOk) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    // Open stream once and read 3 times
    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, readStream(_, _))
        .Times(3)
        .WillRepeatedly([](uint8_t* dst, const size_t& len) -> int {
            std::memset(dst, 0xAA, len);
            return static_cast<int>(len);
        });
    common::MockStopToken token;
    configureNonStoppingToken(token);
    ASSERT_NE(httpFn, nullptr);
    // 3 http steps
    EXPECT_CALL(token, stopRequested()).Times(6).WillRepeatedly(Return(false));
    common::StepResult r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);
    r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Continue);

    // Prepare RB
    // 1) Fill to full first
    // 2) Read to 127 to have 2 spans
    // 3) Fill some data
    std::vector<uint8_t> data1(116U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data1.data(), data1.size()), data1.size() - 1);
    fakeRing->commitRead(127U * 1024U);
    std::vector<uint8_t> data2(90U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data2.data(), data2.size()), data2.size());

    // Fail
    common::Mp3FrameInfo info1{.frameBytes = 0, .hz = 44100, .channels = 2, .samplesPerCh = 1152};
    EXPECT_CALL(*mockMp3Decoder, decode(_, 1024, _, _)).WillOnce(Return(info1));

    common::Mp3FrameInfo info2{
        .frameBytes = adapters::MaxBytesPerFrame, .hz = 44100, .channels = 2, .samplesPerCh = 1152};
    EXPECT_CALL(*mockMp3Decoder, decode(_, 4096, _, _)).WillOnce(Return(info2));
    EXPECT_CALL(*mockI2sBus, getSampleRate()).WillOnce(Return(44100));
    EXPECT_CALL(*mockI2sBus, write(_, 2304, 600)).Times(2).WillRepeatedly(Return(2304));
    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Continue);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Playing);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc13_setVolume) {
    playerService->setVolume(50);
    EXPECT_EQ(playerService->getVolumeQ15(), 16383);

    playerService->setVolume(10);
    EXPECT_EQ(playerService->getVolumeQ15(), 3276);
}

TEST_F(PlayerServiceTest, tc14_playStation_streamStall_reopenAndRetry) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    {
        InSequence seq;
        EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
        EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHttpClient, readStream(_, _)).Times(17).WillRepeatedly(Return(0));
        EXPECT_CALL(*mockHttpClient, closeStream()).Times(1);
        EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
        EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHttpClient, readStream(_, _)).WillOnce(Return(0));
    }

    common::MockStopToken token;
    configureNonStoppingToken(token);
    EXPECT_CALL(token, stopRequested()).Times(36).WillRepeatedly(Return(false));

    ASSERT_NE(httpFn, nullptr);
    for (int i = 0; i < 16; ++i) {
        const common::StepResult r = httpFn(httpUser, token);
        EXPECT_EQ(r.action, common::StepAction::Sleep);
        EXPECT_EQ(r.sleepMs, 300U);
    }

    const common::StepResult reconnectStep = httpFn(httpUser, token);
    EXPECT_EQ(reconnectStep.action, common::StepAction::Sleep);
    EXPECT_EQ(reconnectStep.sleepMs, 500U);

    const common::StepResult afterReconnect = httpFn(httpUser, token);
    EXPECT_EQ(afterReconnect.action, common::StepAction::Sleep);
    EXPECT_EQ(afterReconnect.sleepMs, 300U);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Buffering);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}
