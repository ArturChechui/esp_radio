/**
 * @file PlayerService.hpp
 * @brief Implementation of the IPlayerService interface for audio streaming.
 *
 * This file contains the PlayerService class, which manages the lifecycle of
 * an audio stream, including data fetching, decoding, and I2S output.
 */

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

/**
 * @namespace adapters
 * @brief Contains hardware and protocol abstraction layer interfaces.
 */
namespace adapters {
class IMp3Decoder;
class II2sBus;
class IHttpClient;
}  // namespace adapters

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {
class IEventQueue;
class ITaskRunner;
class IAudioBufferStats;
class ISignal;
}  // namespace common

/**
 * @namespace services
 * @brief Contains business logic service implementations.
 */
namespace services {

/**
 * @class PlayerService
 * @brief Concrete implementation of the audio player engine.
 *
 * PlayerService implements a producer-consumer architecture for audio playback.
 * An HTTP task (the producer) fetches MP3 data and writes it to a ring buffer,
 * while a playback task (the consumer) reads from the buffer, decodes the MP3
 * frames, and writes the resulting PCM samples to the I2S bus.
 */
class PlayerService : public IPlayerService {
   public:
    /**
     * @brief Constructs a PlayerService with all required dependencies.
     * @param i2sBus Reference to the I2S hardware adapter for audio output.
     * @param httpClient Reference to the HTTP client for stream acquisition.
     * @param mp3Decoder Reference to the decoder for transforming MP3 to PCM.
     * @param runner The task runner used to spawn background processing threads.
     * @param ringBuffer Thread-safe buffer for raw bitstream data.
     * @param stats Utility for tracking and reporting buffer/timing statistics.
     * @param coreEventQueue Queue for sending status updates to the application core.
     * @param semaphore Signal used for synchronization between producer and consumer.
     */
    explicit PlayerService(adapters::II2sBus& i2sBus, adapters::IHttpClient& httpClient,
                           adapters::IMp3Decoder& mp3Decoder, common::ITaskRunner& runner,
                           std::unique_ptr<common::IRingBuffer> ringBuffer,
                           common::IAudioBufferStats& stats, common::IEventQueue& coreEventQueue,
                           std::unique_ptr<common::ISignal> semaphore);

    /**
     * @brief Destroys the PlayerService, ensuring all background tasks are stopped.
     */
    ~PlayerService() override;

    /**
     * @brief Starts playback of a radio station from a given URL.
     * * This resets the engine, clears buffers, and spawns the HTTP producer
     * and audio consumer tasks.
     * @param url The URL of the MP3 stream.
     * @return true if playback tasks were successfully started.
     */
    bool playStation(const std::string& url) override;

    /**
     * @brief Halts the current playback and shuts down the background threads.
     * @return true if the player was successfully stopped.
     */
    bool stop() override;

    /**
     * @brief Returns the current status of the playback engine.
     * @return common::PlaybackStatus (e.g., Playing, Stopped, Error).
     */
    common::PlaybackStatus getStatus() const override;

    /**
     * @brief Returns the URL of the stream currently being played.
     * @return The active URL string.
     */
    std::string getCurrentUrl() const override;

    /**
     * @brief Gets the current volume in Q15 fixed-point format.
     * @return The volume level as an int32_t.
     */
    int32_t getVolumeQ15() const override;

    /**
     * @brief Sets the system volume level.
     * @param vol Integer volume value (typically 0-255).
     */
    void setVolume(const uint8_t vol) override;

    /** @brief Constant defining the size of the internal audio ring buffer. */
    static constexpr size_t RingBufferSize = 128U * 1024U;

   private:
    /**
     * @brief Internal helper to notify the system core of a playback state change.
     * @param status The new playback status.
     */
    void onPlaybackStatusChanged(const common::PlaybackStatus& status);

    /**
     * @brief Static entry point for the HTTP producer task.
     */
    static common::StepResult producerStepFn(void* arg, common::IStopToken& token);

    /**
     * @brief Logic for the HTTP producer task, fetching data into the ring buffer.
     */
    common::StepResult producerStep(common::IStopToken& token);

    /**
     * @brief Closes stream and clears RB
     */
    void shutdownStream();

    /**
     * @brief Ensures the stream is active and ready, returning a result if the task must yield
     * or abort.
     */
    std::optional<common::StepResult> ensureStreamOpen();

    /**
     * @brief Performs a single http read and RB write iteration.
     */
    common::StepResult produceOnce(common::IStopToken& token);

    /**
     * @brief Static entry point for the audio consumer/playback task.
     */
    static common::StepResult consumerStepFn(void* arg, common::IStopToken& token);

    /**
     * @brief Logic for the audio consumer task, reading and decoding MP3 data.
     */
    common::StepResult consumerStep(common::IStopToken& token);

    /**
     * @brief Performs a single decode and output iteration.
     */
    common::StepResult consumeOnce(common::IStopToken& token);

    /**
     * @brief Prepares the input scratch buffer for the decoder from ring buffer spans.
     */
    size_t prepareInputScratch(const common::IRingBuffer::ReadSpans& spans);

    /**
     * @brief Logs current buffer and performance statistics.
     */
    void logStats();

    /** @brief Optimized mono-to-stereo conversion with volume scaling in Q15. */
    static void convertMonoToStereoQ15(const int16_t* mono, int16_t* outStereo, const int samples,
                                       const int32_t volQ15);

    /** @brief Applies volume scaling to a stereo PCM buffer in Q15. */
    static void applyVolumeStereoQ15(int16_t* stereo, const int samplesPerCh, const int32_t volQ15);

    /** @brief Converts a 0-100/0-255 percentage to a Q15 multiplier. */
    static int32_t volumePercentToQ15(const uint8_t volume);

    common::PlaybackStatus mStatus; /**< Current system playback state. */
    std::string mCurrentUrl;        /**< URL of the current stream. */

    common::IEventQueue& mCoreEventQueue; /**< Reference to application event queue. */
    adapters::II2sBus& mI2sBus;           /**< Reference to I2S hardware. */
    adapters::IHttpClient& mHttpClient;   /**< Reference to stream fetcher. */
    adapters::IMp3Decoder& mMp3Decoder;   /**< Reference to audio decoder. */
    common::ITaskRunner& mTaskRunner;     /**< Reference to background task manager. */
    common::IAudioBufferStats& mStats;    /**< Performance monitoring utility. */
    std::unique_ptr<common::ISignal> mStreamOpenSignal; /**< Synchronization for HTTP connection. */

    uint32_t mReadStallMs;                /**< Counter for buffer underrun detection. */
    bool mPlayingNotified;                /**< Tracks if "Started Playing" event was sent. */
    std::atomic<bool> mStreamOpen;        /**< Flag indicating active HTTP stream. */
    std::atomic<bool> mIsPlaying;         /**< Flag indicating active playback thread. */
    common::TaskHandle mHttpTaskHandle;   /**< Handle for the producer thread. */
    common::TaskHandle mPlayerTaskHandle; /**< Handle for the consumer thread. */

    std::unique_ptr<common::IRingBuffer> mRingBuffer; /**< Raw bitstream storage buffer. */
    std::vector<uint8_t> mInputScratch; /**< Buffer only for ring wrap boundary decode. */
    std::vector<int16_t> mPcm;          /**< minimp3 output. */
    std::vector<int16_t> mMonoToStereo; /**< mono->stereo conversion buffer. */

    std::atomic<int32_t> mVolumeQ15; /**< Volume in the Q15 format. */
};

}  // namespace services
