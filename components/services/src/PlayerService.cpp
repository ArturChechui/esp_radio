#include "PlayerService.hpp"

#include <esp_err.h>
#include <esp_log.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "Dumpers.hpp"
#include "Events.hpp"
#include "IAudioBufferStats.hpp"
#include "IEventQueue.hpp"
#include "IHttpClient.hpp"
#include "II2sBus.hpp"
#include "IMp3Decoder.hpp"
#include "IStopToken.hpp"
#include "ITaskRunner.hpp"
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

constexpr size_t ReadMaxBytes = 4 * 1024U;  // Max read from ring buffer per decode
constexpr size_t PrebufferBytes = 8U * 1024U;
constexpr uint32_t FreeSpaceTimeoutMs = 500U;         // for waiting for space
constexpr uint32_t AvailDataTimeoutMs = 200U;         // for waiting for data
constexpr uint32_t NoWaitMs = 0U;                     // for non-blocking calls
constexpr uint32_t LowWaterMarkBytes = 40U * 1024U;   // Speeds up the HTTP task
constexpr uint32_t HighWaterMarkBytes = 60U * 1024U;  // Slows down the HTTP task
// TODO: uint32_t to TickType_t?

constexpr int StereoChannels = 2;
constexpr int MonoChannels = 1;

constexpr size_t I2sChunkBytes = 2304U;  // must be multiple of sample size to avoid partial frames
constexpr uint32_t I2sTimeoutMs = 600U;  // allow blocking/yielding to keep IDLE alive

constexpr size_t InputScratchBytes = 4096U;  // Scratch buffer for wrap-boundary frames
constexpr int32_t VolumeQ15 = 3277;
// 4915;  // 6554;          // Volume in Q15 fixed point (0..32768). 0.20 ~= 6554

constexpr size_t ResyncThresholdBytes = 2048U;  // if more, try to resync MP3 frame
constexpr size_t MaxNoDataReads = 100U;  // after this many zero-byte reads, consider stream ended
}  // namespace

PlayerService::PlayerService(adapters::II2sBus& i2sBus, adapters::IHttpClient& httpClient,
                             adapters::IMp3Decoder& mp3Decoder, common::ITaskRunner& runner,
                             std::unique_ptr<common::IRingBuffer> ringBuffer,
                             common::IAudioBufferStats& stats, common::IEventQueue& coreEventQueue,
                             std::unique_ptr<common::ISignal> semaphore)
    : mStatus(common::PlaybackStatus::Idle),
      mCurrentUrl(""),
      mCoreEventQueue(coreEventQueue),
      mI2sBus(i2sBus),
      mHttpClient(httpClient),
      mMp3Decoder(mp3Decoder),
      mTaskRunner(runner),
      mStats(stats),
      mStreamOpenSignal(std::move(semaphore)),
      mNoDataCount(0U),
      mPlayingNotified(false),
      mIsPlaying(false),
      mHttpTaskHandle(),
      mPlayerTaskHandle(),
      mRingBuffer(std::move(ringBuffer)),
      mInputScratch(InputScratchBytes),
      mPcm(adapters::MaxSamplesPerFrame),
      mMonoToStereo(adapters::MaxSamplesPerFrame * StereoChannels) {
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
    mNoDataCount = 0U;
    mPlayingNotified = false;
    mCurrentUrl.clear();
    mStatus = common::PlaybackStatus::Idle;

    if (mHttpTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mHttpTaskHandle, TimeoutToExitTasks);
        mHttpTaskHandle = {};
    }
    if (mPlayerTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mPlayerTaskHandle, TimeoutToExitTasks);
        mPlayerTaskHandle = {};
    }
}

bool PlayerService::init() {
    ESP_LOGI(Tag, "Initializing PlayerService");
    // TBD
    ESP_LOGI(Tag, "PlayerService initialized successfully");
    return true;
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
    mNoDataCount = 0U;
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
    mNoDataCount = 0U;

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

    mNoDataCount = 0U;
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
        ESP_LOGE(Tag, "HTTP openStream failed. Exiting producer step");
        onPlaybackStatusChanged(common::PlaybackStatus::Error);

        shutdownStream();

        return common::StepResult{.action = common::StepAction::Error};
    }

    mNoDataCount = 0U;
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
        (void)token.sleepMs(30U);
    }

    const auto spans = mRingBuffer->claimWriteSpans(ReadMaxBytes);
    if (spans.first.len == 0UL) {
        return {.action = common::StepAction::Sleep, .sleepMs = 10U};
    }

    const int bytesRead = mHttpClient.readStream(spans.first.ptr, spans.first.len);
    mStats.onHttpRead(bytesRead);

    if (bytesRead > 0) {
        mRingBuffer->commitWrite(static_cast<size_t>(bytesRead));
        mNoDataCount = 0U;
        return {.action = common::StepAction::Continue};
    }

    if (bytesRead == 0) {
        if (++mNoDataCount > MaxNoDataReads) {
            ESP_LOGW(Tag, "HTTP: no data for too long, stopping stream");
            shutdownStream();
            // !!!!!!!!!!!!!!!!!!! Reopen so it works better
            return {.action = common::StepAction::Error};
        }
        const uint32_t backoffMs = std::min<uint32_t>(30000U, 10U * mNoDataCount);
        return {.action = common::StepAction::Sleep, .sleepMs = backoffMs};
    }

    ESP_LOGW(Tag, "HTTP read error: %d", bytesRead);
    shutdownStream();
    // TODO: try to reopen the stream and read again when error?
    return {.action = common::StepAction::Error};
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

    if (!mStreamOpen.load(std::memory_order_acquire)) {
        (void)mStreamOpenSignal->wait(200U);
        return {.action = common::StepAction::Continue};
    }

    if (mRingBuffer->available() < PrebufferBytes) {
        onPlaybackStatusChanged(common::PlaybackStatus::Buffering);
        (void)mRingBuffer->waitForData(AvailDataTimeoutMs);
        // TODO: instead of a big sleep, wait for a specific about of data here, like
        // 2xPrebufferBytes so that it doesn't have glitches when 1 frame is downloaded and it goes
        return {.action = common::StepAction::Sleep, .sleepMs = 2000U};
    }

    return consumeOnce(token);
}

common::StepResult PlayerService::consumeOnce(common::IStopToken& token) {
    const auto lv = mRingBuffer->getFillLevels();
    mStats.observeRing(lv.avail, lv.space);

    const auto spans = mRingBuffer->claimReadSpans(ReadMaxBytes);
    if (spans.total() < 4UL) {
        (void)mRingBuffer->waitForData(AvailDataTimeoutMs);
        return {.action = common::StepAction::Sleep, .sleepMs = 10U};
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
        } else {
            (void)mRingBuffer->waitForData(AvailDataTimeoutMs);
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

    const int16_t* outSamples = nullptr;
    if (info.channels == MonoChannels) {
        convertMonoToStereoQ15(mPcm.data(), mMonoToStereo.data(), info.samplesPerCh, VolumeQ15);
        outSamples = mMonoToStereo.data();
    } else {
        applyVolumeStereoQ15(mPcm.data(), info.samplesPerCh, VolumeQ15);
        outSamples = mPcm.data();
    }

    onPlaybackStatusChanged(common::PlaybackStatus::Playing);

    const size_t bytesToWrite =
        static_cast<size_t>(info.samplesPerCh) * StereoChannels * sizeof(outSamples[0]);
    const uint8_t* outBytes = reinterpret_cast<const uint8_t*>(outSamples);
    // TODO: improve uint8 and uint16 convertion

    size_t writtenTotalBytes = 0UL;
    uint8_t zeroWrites = 0U;
    while (writtenTotalBytes < bytesToWrite && !token.stopRequested()) {
        const size_t chunk = std::min(I2sChunkBytes, (bytesToWrite - writtenTotalBytes));
        const size_t written = mI2sBus.write(
            reinterpret_cast<const int16_t*>(outBytes + writtenTotalBytes), chunk, I2sTimeoutMs);

        mStats.onI2sWrite(written, (written == 0U), ((written > 0U) && (written < chunk)));

        if (written == 0U) {
            if (++zeroWrites >= 3U) {
                return {.action = common::StepAction::Sleep, .sleepMs = 10U};
            }

            (void)token.sleepMs(10U);
            continue;
        }

        writtenTotalBytes += written;
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
        x >>= 15;

        const int16_t v = static_cast<int16_t>(x);
        outStereo[i * 2] = v;
        outStereo[i * 2 + 1] = v;
    }
}

void PlayerService::applyVolumeStereoQ15(int16_t* stereo, const int samplesPerCh,
                                         const int32_t volQ15) {
    const int total = (samplesPerCh * StereoChannels);
    for (int i = 0; i < total; ++i) {
        // Q15 multiply: (s * volQ15) >> 15
        int32_t x = static_cast<int32_t>(stereo[i]) * volQ15;
        x >>= 15;

        stereo[i] = static_cast<int16_t>(x);
    }
}

}  // namespace services
