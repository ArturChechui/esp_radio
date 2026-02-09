#include "PlayerServiceTest.hpp"

#include "FakeBinarySemaphore.hpp"
#include "FakeRingBuffer.hpp"
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
        *mockEventQueue, std::make_unique<common::FakeBinarySemaphore>());
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

TEST_F(PlayerServiceTest, tc01_init_returnsTrue) {
    EXPECT_TRUE(playerService->init());
}

TEST_F(PlayerServiceTest, tc02_playStation_emptyUrl_returnsFalse) {
    EXPECT_TRUE(playerService->init());

    EXPECT_FALSE(playerService->playStation(""));
    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Idle);
}

TEST_F(PlayerServiceTest, tc03_playStation_success_capturesStepFns_andSetsBuffering) {
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

TEST_F(PlayerServiceTest, tc04_playStation_playerTaskStartFails_stopsHttpTask_andSetsError) {
    InSequence seq;

    const common::TaskHandle httpHandle{0, 1};
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _)).WillOnce(Return(httpHandle));  // HttpTask ok
    EXPECT_CALL(*mockTaskRunner, start(_, _, _, _))
        .WillOnce(Return(common::TaskHandle{}));  // PlayerTask fails

    // Failure path stops HTTP task once
    EXPECT_CALL(*mockTaskRunner, stop(_, _)).WillOnce(Return(common::StopResult::Ok));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));

    EXPECT_FALSE(playerService->playStation("http://example.com/stream.mp3"));
    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Error);
}

TEST_F(PlayerServiceTest, tc05_producerStepFn_openStreamFails_shutsDownAndReturnsError) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://bad.url/stream.mp3"));

    // ensureStreamOpen(), fail -> closeStream
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

    // Since shutdownStream() sets mIsPlaying=false, consumer step should stop immediately
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    ASSERT_NE(playerFn, nullptr);
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Done);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc06_consumerStepFn_whenStreamNotOpen_returnsContinue) {
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

TEST_F(PlayerServiceTest, tc07_consumerStepFn_whenNotPrebuffered_returnsSleep100) {
    expectStartCaptureBothStepFns();

    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    // First run producer entrypoint once so it opens the stream (mStreamOpen=true).
    EXPECT_CALL(*mockHttpClient, isStreamOpen()).WillOnce(Return(false));
    EXPECT_CALL(*mockHttpClient, openStream(_, _)).WillOnce(Return(true));
    // produceOnce() will try to read; return 0 to keep fakeRing empty
    EXPECT_CALL(*mockHttpClient, readStream(_, _)).WillOnce(Return(0));
    common::MockStopToken token;
    configureNonStoppingToken(token);
    EXPECT_CALL(token, stopRequested()).Times(2).WillRepeatedly(Return(false));
    ASSERT_NE(httpFn, nullptr);
    const common::StepResult r1 = httpFn(httpUser, token);
    EXPECT_EQ(r1.action, common::StepAction::Sleep);
    EXPECT_EQ(r1.sleepMs, 10U);

    // Now consumer sees stream open but fakeRing < PrebufferBytes -> Sleep 2000ms
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

TEST_F(PlayerServiceTest, tc08_consumerStepFn_decodeFrame0_resyncDropsOneByte) {
    expectStartCaptureBothStepFns();
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    ASSERT_TRUE(playerService->playStation("http://example.com/stream.mp3"));

    // Open stream once
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
    // Pre-fill fakeRing > PrebufferBytes and enough so spans.total() > ResyncThreshold
    std::vector<uint8_t> data(12U * 1024U, 0xAA);
    ASSERT_EQ(fakeRing->push(data.data(), data.size()), data.size());
    const size_t before = fakeRing->available();
    ASSERT_GT(before, 8192U);

    // Decoder: frameBytes==0 -> resync path should commitRead(1) when spans.total > threshold
    common::Mp3FrameInfo info{};
    info.frameBytes = 0;
    EXPECT_CALL(*mockMp3Decoder, decode(_, _, _, _)).WillOnce(Return(info));

    ASSERT_NE(playerFn, nullptr);
    EXPECT_CALL(token, stopRequested()).WillOnce(Return(false));
    EXPECT_CALL(*mockEventQueue, post(_)).WillOnce(Return(true));
    const common::StepResult r2 = playerFn(playerUser, token);
    EXPECT_EQ(r2.action, common::StepAction::Continue);

    const size_t after = fakeRing->available();
    EXPECT_EQ(after, before - 1U);

    EXPECT_EQ(playerService->getStatus(), common::PlaybackStatus::Playing);

    // Destructor, 2 tasks
    EXPECT_CALL(*mockTaskRunner, stop(_, _))
        .Times(::testing::Exactly(2))
        .WillRepeatedly(::testing::Return(common::StopResult::Ok));
}

TEST_F(PlayerServiceTest, tc09_fullSteps_Success) {
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