#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "IPlayerService.hpp"
#include "IRingBuffer.hpp"
#include "ISignal.hpp"
#include "Types.hpp"

namespace adapters {
class IMp3Decoder;
class II2sBus;
class IHttpClient;
}  // namespace adapters

namespace common {
class IEventQueue;
class ITaskRunner;
class IAudioBufferStats;
class ISignal;
}  // namespace common

namespace services {
class PlayerService : public IPlayerService {
   public:
    explicit PlayerService(adapters::II2sBus& i2sBus, adapters::IHttpClient& httpClient,
                           adapters::IMp3Decoder& mp3Decoder, common::ITaskRunner& runner,
                           std::unique_ptr<common::IRingBuffer> ringBuffer,
                           common::IAudioBufferStats& stats, common::IEventQueue& coreEventQueue,
                           std::unique_ptr<common::ISignal> semaphore);
    ~PlayerService() override;

    bool playStation(const std::string& url) override;
    bool stop() override;
    common::PlaybackStatus getStatus() const override;
    std::string getCurrentUrl() const override;
    int32_t getVolumeQ15() const override;
    void setVolume(const uint8_t vol) override;

    static constexpr size_t RingBufferSize = 64U * 1024U;

   private:
    void onPlaybackStatusChanged(const common::PlaybackStatus& status);

    static common::StepResult producerStepFn(void* arg, common::IStopToken& token);

    common::StepResult producerStep(common::IStopToken& token);
    void shutdownStream();
    std::optional<common::StepResult> ensureStreamOpen();
    common::StepResult produceOnce(common::IStopToken& token);

    static common::StepResult consumerStepFn(void* arg, common::IStopToken& token);
    common::StepResult consumerStep(common::IStopToken& token);
    common::StepResult consumeOnce(common::IStopToken& token);
    size_t prepareInputScratch(const common::IRingBuffer::ReadSpans& spans);
    void logStats();
    static void convertMonoToStereoQ15(const int16_t* mono, int16_t* outStereo, const int samples,
                                       const int32_t volQ15);
    static void applyVolumeStereoQ15(int16_t* stereo, const int samplesPerCh, const int32_t volQ15);
    static int32_t volumePercentToQ15(const uint8_t volume);

    common::PlaybackStatus mStatus;
    std::string mCurrentUrl;

    common::IEventQueue& mCoreEventQueue;
    adapters::II2sBus& mI2sBus;
    adapters::IHttpClient& mHttpClient;
    adapters::IMp3Decoder& mMp3Decoder;
    common::ITaskRunner& mTaskRunner;
    common::IAudioBufferStats& mStats;
    std::unique_ptr<common::ISignal> mStreamOpenSignal;

    uint32_t mReadStallMs;
    bool mPlayingNotified;
    std::atomic<bool> mStreamOpen;
    std::atomic<bool> mIsPlaying;
    common::TaskHandle mHttpTaskHandle;
    common::TaskHandle mPlayerTaskHandle;

    std::unique_ptr<common::IRingBuffer> mRingBuffer;
    std::vector<uint8_t> mInputScratch;  // only for ring wrap boundary decode
    std::vector<int16_t> mPcm;           // minimp3 output
    std::vector<int16_t> mMonoToStereo;  // mono->stereo conversion buffer

    std::atomic<int32_t> mVolumeQ15;
};

}  // namespace services
