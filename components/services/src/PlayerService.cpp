#include "PlayerService.hpp"

#include <esp_err.h>
#include <esp_log.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "Dumpers.hpp"
#include "Events.hpp"
#include "IEventQueue.hpp"
#include "IHttpClient.hpp"
#include "II2sBus.hpp"
#include "IMp3Decoder.hpp"
#include "IStopToken.hpp"
#include "ITaskRunner.hpp"
#include "Signal.hpp"
#include "Types.hpp"

namespace services {
namespace {
constexpr const char* Tag = "PlayerService";

constexpr int PlayerTaskPriority = 10;  // 10 means max priority
constexpr int PlayerTaskCore = 1;
constexpr int HttpTaskPriority = 9;  // slightly lower than player to favor audio
constexpr int HttpTaskCore = 0;
constexpr uint32_t TimeoutToExitTasks = 7000U;  // 7s
constexpr uint32_t PlayerTaskStackWords = 22480U;
constexpr uint32_t HttpTaskStackWords = 8192U;

constexpr size_t MinMp3FrameSize =
    4UL;  // Minimum MP3 frame size in bytes (to avoid decoding errors)
constexpr size_t ReadMaxBytes = 4 * 1024U;  // Max read from ring buffer per decode
constexpr size_t PrebufferBytes = 32U * 1024U;
constexpr size_t ResumeBufferBytes = 5U * PrebufferBytes;
constexpr uint32_t FreeSpaceTimeoutMs = 500U;   // for waiting for space
constexpr uint32_t AvailDataTimeoutMs = 200U;   // for waiting for data
constexpr uint32_t StreamOpenTimeoutMs = 130U;  // for waiting stream open signal
constexpr uint32_t LowWaterMarkBytes =
    PlayerService::RingBufferSize * 0.7;  // Speeds up the HTTP task before the buffer hits 50 %
constexpr uint32_t HighWaterMarkBytes =
    PlayerService::RingBufferSize * 0.9;  // Slows down the HTTP task after it hits 90%

constexpr int StereoChannels = 2;
constexpr int MonoChannels = 1;

constexpr size_t I2sChunkSamples = 1152U;  // Max stereo sample chunk for I2S writes (1 MP3 frame)
constexpr uint32_t I2sTimeoutMs = 600U;    // allow blocking/yielding to keep IDLE alive

constexpr size_t InputScratchBytes = 4096U;  // Scratch buffer for wrap-boundary frames
constexpr int32_t InitVolQ15 = 3277;         // Volume in Q15 fixed point (0..32768). 0.10 ~= 3277

constexpr size_t ResyncThresholdBytes = 2048U;  // if more, try to resync MP3 frame
constexpr uint32_t StreamReadRetrySleepMs = 300U;
constexpr uint32_t StreamOpenRetrySleepMs = 500U;
constexpr uint32_t ReconnectAfterReadStallMs = 5000U;
constexpr uint32_t BufferingPollSleepMs = 20U;

constexpr int32_t Q15One = 0x7FFF;  // 32767 - represents 1.0 in Q15 fixed-point format

constexpr uint32_t DefaultStatsIntervalMs = 10000U;
}  // namespace

PlayerService::PlayerService(adapters::II2sBus& i2sBus, adapters::IHttpClient& httpClient,
                             adapters::IMp3Decoder& mp3Decoder, common::ITaskRunner& runner,
                             std::unique_ptr<common::IRingBuffer> ringBuffer,
                             common::IEventQueue& coreEventQueue)
    : mStatus(common::PlaybackStatus::Idle),
      mCurrentUrl(""),
      mCoreEventQueue(coreEventQueue),
      mI2sBus(i2sBus),
      mHttpClient(httpClient),
      mMp3Decoder(mp3Decoder),
      mTaskRunner(runner),
      mStreamOpenSignal(std::make_unique<common::Signal>()),
      mReadStallMs(0U),
      mPlayingNotified(false),
      mIsPlaying(false),
      mHttpTaskHandle(),
      mPlayerTaskHandle(),
      mRingBuffer(std::move(ringBuffer)),
      mInputScratch(InputScratchBytes),
      mPcm(adapters::MaxSamplesPerFrame),
      mMonoToStereo(adapters::MaxSamplesPerFrame * StereoChannels),
      mVolumeQ15(InitVolQ15),
      mStats(DefaultStatsIntervalMs) {
    ESP_LOGI(Tag, "PlayerService created");
}

PlayerService::~PlayerService() {
    ESP_LOGI(Tag, "Destructing PlayerService");

    mIsPlaying.store(false);
    mRingBuffer->abort();
    if (mStreamOpenSignal) {
        mStreamOpenSignal->signal();
    }
    mStreamOpen.store(false);
    mReadStallMs = 0U;
    mPlayingNotified = false;
    mCurrentUrl.clear();
    mStatus = common::PlaybackStatus::Idle;

    if (mHttpTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mHttpTaskHandle, TimeoutToExitTasks);
        mHttpTaskHandle.reset();
    }
    if (mPlayerTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mPlayerTaskHandle, TimeoutToExitTasks);
        mPlayerTaskHandle.reset();
    }
}

bool PlayerService::playStation(const std::string& url) {
    if (url.empty()) {
        ESP_LOGE(Tag, "Empty URL");
        return false;
    }

    if (mIsPlaying.load()) {
        ESP_LOGW(Tag, "Already playing %s", mCurrentUrl.c_str());
        return false;
    }

    mCurrentUrl = url;
    ESP_LOGI(Tag, "Playing station: %s", mCurrentUrl.c_str());

    mStreamOpen.store(false);
    mStreamOpenSignal->reset();
    mIsPlaying.store(true);
    mRingBuffer->reset();
    mReadStallMs = 0U;
    mPlayingNotified = false;

    onPlaybackStatusChanged(common::PlaybackStatus::Buffering);

    mHttpTaskHandle = mTaskRunner.start(
        common::TaskParams{.name = "HttpTask", .priority = HttpTaskPriority, .core = HttpTaskCore},
        HttpTaskStackWords, &PlayerService::producerStepFn, this);
    if (!mHttpTaskHandle.isValid()) {
        ESP_LOGE(Tag, "Failed to create HttpTask");
        mIsPlaying.store(false);
        onPlaybackStatusChanged(common::PlaybackStatus::Error);
        return false;
    }

    mPlayerTaskHandle = mTaskRunner.start(
        common::TaskParams{
            .name = "PlayerTask", .priority = PlayerTaskPriority, .core = PlayerTaskCore},
        PlayerTaskStackWords, &PlayerService::consumerStepFn, this);
    if (!mPlayerTaskHandle.isValid()) {
        ESP_LOGE(Tag, "Failed to create PlayerTask");
        mIsPlaying.store(false);

        // unblock HTTP
        mRingBuffer->abort();
        (void)mTaskRunner.stop(mHttpTaskHandle, TimeoutToExitTasks);
        mHttpTaskHandle = {};
        mStreamOpenSignal->signal();

        onPlaybackStatusChanged(common::PlaybackStatus::Error);
        return false;
    }

    ESP_LOGI(Tag, "Tasks started: Player%s, Http%s", common::dump(mPlayerTaskHandle).c_str(),
             common::dump(mHttpTaskHandle).c_str());
    return true;
}

bool PlayerService::stop() {
    if (mStatus == common::PlaybackStatus::Idle || mStatus == common::PlaybackStatus::Stopped) {
        ESP_LOGW(Tag, "Not playing, nothing to stop");
        return true;
    }

    if (!mPlayerTaskHandle.isValid() && !mHttpTaskHandle.isValid()) {
        ESP_LOGW(Tag, "Tasks are not running, nothing to stop");
        return true;
    }

    ESP_LOGI(Tag, "Stopping playback");

    mIsPlaying.store(false);
    mRingBuffer->abort();
    mStreamOpenSignal->signal();
    mStreamOpen.store(false);
    mPlayingNotified = false;
    mReadStallMs = 0U;

    (void)mTaskRunner.stop(mHttpTaskHandle, TimeoutToExitTasks);
    (void)mTaskRunner.stop(mPlayerTaskHandle, TimeoutToExitTasks);
    mHttpTaskHandle.reset();
    mPlayerTaskHandle.reset();

    onPlaybackStatusChanged(common::PlaybackStatus::Stopped);
    mCurrentUrl.clear();
    return true;
}

common::PlaybackStatus PlayerService::getStatus() const {
    return mStatus;
}

std::string PlayerService::getCurrentUrl() const {
    return mCurrentUrl;
}

int32_t PlayerService::getVolumeQ15() const {
    return mVolumeQ15.load(std::memory_order_relaxed);
}

void PlayerService::setVolume(const uint8_t vol) {
    ESP_LOGI(Tag, "setVolume(%u)", vol);

    const int32_t volQ15 = volumePercentToQ15(vol);
    mVolumeQ15.store(volQ15, std::memory_order_relaxed);
}

void PlayerService::onPlaybackStatusChanged(const common::PlaybackStatus& status) {
    if (mStatus == status) {
        return;
    }

    ESP_LOGI(Tag, "Status changed: %s -> %s", common::dump(mStatus).c_str(),
             common::dump(status).c_str());
    mStatus = status;

    mCoreEventQueue.post(common::PlaybackStatusChangedEvent{status});
}

common::StepResult PlayerService::producerStepFn(void* arg, common::IStopToken& token) {
    auto* self = static_cast<PlayerService*>(arg);
    if (!self) {
        return {.action = common::StepAction::Error};
    }

    return self->producerStep(token);
}

common::StepResult PlayerService::producerStep(common::IStopToken& token) {
    if (token.stopRequested() || !mIsPlaying.load(std::memory_order_acquire)) {
        shutdownStream();
        ESP_LOGI(Tag, "Producer step exiting");
        return {.action = common::StepAction::Done};
    }

    const auto openRes = ensureStreamOpen();
    if (openRes.has_value()) {
        return openRes.value();
    }

    return produceOnce(token);
}

void PlayerService::shutdownStream() {
    mHttpClient.closeStream();

    mReadStallMs = 0U;
    mStreamOpen.store(false, std::memory_order_release);
    mStreamOpenSignal->signal();

    mIsPlaying.store(false);
    mRingBuffer->abort();
}

std::optional<common::StepResult> PlayerService::ensureStreamOpen() {
    if (mStreamOpen.load(std::memory_order_acquire)) {
        return std::nullopt;
    }

    if (mHttpClient.isStreamOpen()) {
        mHttpClient.closeStream();
    }

    if (!mHttpClient.openStream(mCurrentUrl, adapters::IHttpClient::DefaultStreamTimeoutMs)) {
        ESP_LOGW(Tag, "HTTP openStream failed, retrying");
        mStreamOpen.store(false, std::memory_order_release);
        return common::StepResult{.action = common::StepAction::Sleep,
                                  .sleepMs = StreamOpenRetrySleepMs};
    }

    mReadStallMs = 0U;
    mStreamOpen.store(true, std::memory_order_release);
    mStreamOpenSignal->signal();

    return std::nullopt;
}

common::StepResult PlayerService::produceOnce(common::IStopToken& token) {
    if (token.stopRequested() || !mIsPlaying.load(std::memory_order_acquire)) {
        shutdownStream();
        ESP_LOGI(Tag, "Producer step exiting");
        return {.action = common::StepAction::Done};
    }

    if (!mRingBuffer->waitForSpace(FreeSpaceTimeoutMs)) {
        return {.action = common::StepAction::Sleep, .sleepMs = 10U};
    }

    const size_t fillBytes = mRingBuffer->available();
    if (fillBytes > HighWaterMarkBytes) {
        return {.action = common::StepAction::Sleep, .sleepMs = 10U};
    } else if (fillBytes > LowWaterMarkBytes) {
        if (token.sleepMs(30U)) {
            return {.action = common::StepAction::Done};
        }
    }

    const auto spans = mRingBuffer->claimWriteSpans(ReadMaxBytes);
    if (spans.first.len == 0UL) {
        return {.action = common::StepAction::Sleep, .sleepMs = 10U};
    }

    const int bytesRead = mHttpClient.readStream(spans.first.ptr, spans.first.len);
    mStats.onHttpRead(bytesRead);

    if (bytesRead > 0) {
        mRingBuffer->commitWrite(static_cast<size_t>(bytesRead));
        mReadStallMs = 0U;
        return {.action = common::StepAction::Continue};
    }

    if (bytesRead < 0) {
        ESP_LOGW(Tag, "HTTP read error: %d", bytesRead);
    }

    mReadStallMs =
        std::min<uint32_t>(ReconnectAfterReadStallMs, mReadStallMs + StreamReadRetrySleepMs);
    if (mReadStallMs >= ReconnectAfterReadStallMs) {
        ESP_LOGW(Tag, "HTTP stream stalled for %u ms, reopening", (unsigned)mReadStallMs);

        mHttpClient.closeStream();
        mReadStallMs = 0U;
        mStreamOpen.store(false, std::memory_order_release);
        mStreamOpenSignal->reset();

        return {.action = common::StepAction::Sleep, .sleepMs = StreamOpenRetrySleepMs};
    }

    return {.action = common::StepAction::Sleep, .sleepMs = StreamReadRetrySleepMs};
}

common::StepResult PlayerService::consumerStepFn(void* arg, common::IStopToken& token) {
    auto* self = static_cast<PlayerService*>(arg);
    if (!self) {
        return common::StepResult{common::StepAction::Error, 0U};
    }

    return self->consumerStep(token);
}

common::StepResult PlayerService::consumerStep(common::IStopToken& token) {
    if (token.stopRequested() || !mIsPlaying.load(std::memory_order_acquire)) {
        ESP_LOGI(Tag, "Consumer step exiting");
        return {.action = common::StepAction::Done};
    }

    const size_t availableBytes = mRingBuffer->available();
    if (!mStreamOpen.load(std::memory_order_acquire) && (availableBytes == 0UL)) {
        if (!mStreamOpenSignal->wait(StreamOpenTimeoutMs)) {
            return {.action = common::StepAction::Sleep, .sleepMs = BufferingPollSleepMs};
        }
        return {.action = common::StepAction::Continue};
    }

    const size_t requiredBufferBytes =
        (mStatus == common::PlaybackStatus::Buffering) ? ResumeBufferBytes : PrebufferBytes;
    if (availableBytes < requiredBufferBytes) {
        onPlaybackStatusChanged(common::PlaybackStatus::Buffering);

        if (!mRingBuffer->waitForData(AvailDataTimeoutMs)) {
            return {.action = common::StepAction::Sleep, .sleepMs = BufferingPollSleepMs};
        }
        return {.action = common::StepAction::Continue};
    }

    return consumeOnce(token);
}

common::StepResult PlayerService::consumeOnce(common::IStopToken& token) {
    recordRingStats();

    const auto spans = mRingBuffer->claimReadSpans(ReadMaxBytes);
    if (spans.total() < MinMp3FrameSize) {
        if (!mRingBuffer->waitForData(AvailDataTimeoutMs)) {
            return {.action = common::StepAction::Sleep, .sleepMs = BufferingPollSleepMs};
        }
        return {.action = common::StepAction::Continue};
    }

    common::Mp3FrameInfo info =
        mMp3Decoder.decode(spans.first.ptr, spans.first.len, mPcm.data(), mPcm.size());
    if (info.frameBytes == 0 && spans.second.len > 0U) {
        const size_t copiedLen = prepareInputScratch(spans);
        info = mMp3Decoder.decode(mInputScratch.data(), copiedLen, mPcm.data(), mPcm.size());
    }

    if (info.frameBytes == 0) {
        mStats.onDecodeFrameBytesZero();

        if (spans.total() > ResyncThresholdBytes) {
            mRingBuffer->commitRead(1UL);
            mStats.onResyncDrop();
        } else if (!mRingBuffer->waitForData(AvailDataTimeoutMs)) {
            return {.action = common::StepAction::Sleep, .sleepMs = BufferingPollSleepMs};
        }

        return {.action = common::StepAction::Continue};
    }

    mRingBuffer->commitRead(static_cast<size_t>(info.frameBytes));

    if (info.samplesPerCh <= 0) {
        mStats.onZeroSampleFrame();
        return {.action = common::StepAction::Continue};
    }
    if (info.hz <= 0 || info.channels <= 0) {
        mStats.onInvalidFrameInfo();
        return {.action = common::StepAction::Continue};
    }

    mStats.onFrameDecoded();

    if (mI2sBus.getSampleRate() != static_cast<uint32_t>(info.hz)) {
        (void)mI2sBus.reconfigureClock(static_cast<uint32_t>(info.hz));
    }

    const int32_t volQ15 = mVolumeQ15.load(std::memory_order_relaxed);
    const int16_t* outSamples = nullptr;
    if (info.channels == MonoChannels) {
        convertMonoToStereoQ15(mPcm.data(), mMonoToStereo.data(), info.samplesPerCh, volQ15);
        outSamples = mMonoToStereo.data();
    } else {
        applyVolumeStereoQ15(mPcm.data(), info.samplesPerCh, volQ15);
        outSamples = mPcm.data();
    }

    onPlaybackStatusChanged(common::PlaybackStatus::Playing);

    const size_t totalSamples = (static_cast<size_t>(info.samplesPerCh) * StereoChannels);
    size_t writtenSamples = 0UL;
    uint8_t zeroWrites = 0U;
    while (writtenSamples < totalSamples && !token.stopRequested()) {
        const size_t samplesRemaining = (totalSamples - writtenSamples);
        const size_t chunkSamples = std::min(I2sChunkSamples, samplesRemaining);
        const size_t chunkBytes = (chunkSamples * sizeof(int16_t));
        const int16_t* currentChunkPtr = (outSamples + writtenSamples);

        const size_t writtenBytes = mI2sBus.write(currentChunkPtr, chunkBytes, I2sTimeoutMs);

        const size_t writtenChunkSamples = (writtenBytes / sizeof(int16_t));

        mStats.onI2sWrite(writtenBytes, (writtenBytes == 0U),
                          ((writtenBytes > 0U) && (writtenBytes < chunkBytes)));

        if (writtenBytes == 0U) {
            if (++zeroWrites >= 3U) {
                return {.action = common::StepAction::Sleep, .sleepMs = 10U};
            }
            continue;
        }

        zeroWrites = 0U;
        writtenSamples += writtenChunkSamples;
    }

    logStats();
    return {.action = common::StepAction::Continue};
}

size_t PlayerService::prepareInputScratch(const common::IRingBuffer::ReadSpans& spans) {
    const size_t bytesToCopy = std::min(InputScratchBytes, spans.total());

    // Copy first part up to ring end
    size_t copied = 0UL;
    const size_t part1Bytes = std::min(spans.first.len, bytesToCopy);
    std::memcpy(mInputScratch.data(), spans.first.ptr, part1Bytes);
    copied += part1Bytes;

    // Copy second part from ring start
    const size_t remain = (bytesToCopy - copied);
    if (remain > 0UL) {
        const size_t part2Bytes = std::min(spans.second.len, remain);
        std::memcpy(mInputScratch.data() + copied, spans.second.ptr, part2Bytes);
        copied += part2Bytes;
    }

    return copied;
}

void PlayerService::logStats() {
    if (mStats.shouldLog()) {
        const auto s = mStats.snapshotAndReset();

        ESP_LOGI(Tag,
                 "[%ums] ring: avail(now=%u min=%u max=%u) space(min=%u) | "
                 "dec: frames=%u frame0=%u resync=%u inv_info %u sample0 %u | "
                 "i2s: calls=%u timeouts=%u max_to=%u bytes=%u partial=%u max_bytes=%u "
                 "min_bytes=%u | "
                 "http: calls=%u zero=%u err=%u bytes=%u",
                 (unsigned)s.period_ms, (unsigned)s.avail_now, (unsigned)s.min_avail,
                 (unsigned)s.max_avail, (unsigned)s.min_space, (unsigned)s.frames,
                 (unsigned)s.decode_frame0, (unsigned)s.resync_drops,
                 (unsigned)s.invalid_frame_info, (unsigned)s.zero_sample_frames,
                 (unsigned)s.i2s_calls, (unsigned)s.i2s_timeouts,
                 (unsigned)s.i2s_max_consecutive_timeouts, (unsigned)s.i2s_written_bytes,
                 (unsigned)s.i2s_partial_writes, (unsigned)s.i2s_max_written_bytes,
                 (unsigned)s.i2s_min_written_bytes, (unsigned)s.http_calls, (unsigned)s.http_zero,
                 (unsigned)s.http_errors, (unsigned)s.http_bytes);
    }
}

void PlayerService::convertMonoToStereoQ15(const int16_t* mono, int16_t* outStereo,
                                           const int samples, const int32_t volQ15) {
    for (int i = 0; i < samples; ++i) {
        // Q15 multiply: (s * volQ15) >> 15
        int32_t x = static_cast<int32_t>(mono[i]) * volQ15;
        const int16_t v = static_cast<int16_t>(x >> 15);

        *outStereo++ = v;  // Writes Left  channel at outStereo[0], advances ptr to [1]
        *outStereo++ = v;  // Writes Right channel at outStereo[1], advances ptr to [2]
    }
}

void PlayerService::applyVolumeStereoQ15(int16_t* stereo, const int samplesPerCh,
                                         const int32_t volQ15) {
    int16_t* const end = (stereo + (samplesPerCh * StereoChannels));
    while (stereo < end) {
        int32_t x = static_cast<int32_t>(*stereo) * volQ15;
        *stereo++ = static_cast<int16_t>(x >> 15);
    }
}

int32_t PlayerService::volumePercentToQ15(const uint8_t volume) {
    return (static_cast<int32_t>(volume) * Q15One) / 100;
}

}  // namespace services
