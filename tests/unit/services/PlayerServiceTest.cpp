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

    auto rb = std::make_unique<common::FakeRingBuffer>(64U * 1024U);
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
    EXPECT_TRUE(playerService->init());
}

TEST_F(PlayerServiceTest, tc02_playStation_emptyUrl) {
    EXPECT_TRUE(playerService->init());

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
    EXPECT_CALL(*mockHttpClient, closeStream()).Times(1);

    common::MockStopToken token;
    configureNonStoppingToken(token);

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    ASSERT_NE(httpFn, nullptr);
    const common::StepResult r = httpFn(httpUser, token);
    EXPECT_EQ(r.action, common::StepAction::Error);
    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Error);

    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    ASSERT_NE(playerFn, nullptr);
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Done);

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
    EXPECT_EQ(r.action, common::StepAction::Continue);

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
    EXPECT_EQ(r1.sleepMs, 10U);

    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Sleep);
    EXPECT_EQ(r2.sleepMs, 2000U);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc08_playStation_decodeFrame0) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHttpClient, readStream(_, _)).WillOnce(Return(0));
    common::MockStopToken token;
    configureNonStoppingToken(token);
    ASSERT_NE(httpFn, nullptr);
    EXPECT_CALL(token, stopRequested()).Times(2).WillRepeatedly(Return(false));
    const common::StepResult r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Sleep);
    EXPECT_EQ(r1.sleepMs, 10U);

    std::vector<uint8_t> data(12U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data.data(), data.size()), data.size());
    const size_t before = fakeRing->available();
    ASSERT_GT(before, 8192U);

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

    // Prepare RB (fill to full after http task)
    std::vector<uint8_t> data1(52U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data1.data(), data1.size()), data1.size() - 1);
    fakeRing->commitRead(63U * 1024U);
    std::vector<uint8_t> data2(32U * 1024U, 0xAA);
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