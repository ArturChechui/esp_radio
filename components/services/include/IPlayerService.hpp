/**
 * @file IPlayerService.hpp
 * @brief Interface definition for the audio player service.
 *
 * This file defines the abstract interface for managing audio playback,
 * volume, and stream state within the application.
 */

#pragma once

#include <cstdint>
#include <string>

#include "Types.hpp"

/**
 * @namespace services
 * @brief Contains business logic services that coordinate hardware and application state.
 */
namespace services {

/**
 * @class IPlayerService
 * @brief Abstract interface for a playback management service.
 *
 * This service provides high-level control over the audio pipeline. It allows
 * other modules to trigger playback of specific URLs, halt audio, and
 * manipulate output volume without needing to know the details of the
 * underlying decoder or I2S hardware.
 */
class IPlayerService {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IPlayerService() = default;

    /**
     * @brief Initiates playback of an audio stream from the given URL.
     * @param url The string URL of the radio station or audio file.
     * @return true if the playback request was successfully initiated, false otherwise.
     */
    virtual bool playStation(const std::string& url) = 0;

    /**
     * @brief Stops the current audio playback and clears internal buffers.
     * @return true if the playback was successfully stopped.
     */
    virtual bool stop() = 0;

    /**
     * @brief Retrieves the current state of the playback engine.
     * @return common::PlaybackStatus (e.g., Playing, Stopped, Buffering, or Error).
     */
    virtual common::PlaybackStatus getStatus() const = 0;

    /**
     * @brief Retrieves the URL of the station currently being played.
     * @return A string containing the active stream URL.
     */
    virtual std::string getCurrentUrl() const = 0;

    /**
     * @brief Gets the current volume level in Q15 fixed-point format.
     * * Q15 is used here for efficient integer-based audio scaling.
     * @return The volume as a 32-bit integer in Q15 representation.
     */
    virtual int32_t getVolumeQ15() const = 0;

    /**
     * @brief Sets the output volume level.
     * @param vol The target volume level (typically 0-255).
     */
    virtual void setVolume(const uint8_t vol) = 0;
};

}  // namespace services
