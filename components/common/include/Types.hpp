/**
 * @file Types.hpp
 * @brief Global type definitions, enumerations, and data structures.
 *
 * This file centralizes the core domain models and primitive types used for
 * inter-service communication, hardware abstraction, and task management.
 */

#pragma once

#include <stdint.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace common {

/** @name Enumerations
 * Simple value types for state machines and command routing.
 * @{ */

/** @enum CommandType
 * @brief High-level control commands dispatched to the system manager.
 */
enum class CommandType : uint8_t {
    ConnectWifi,  /**< Initiates a connection to a Wi-Fi network. */
    PlayStopSkip, /**< Toggles playback or skips to the next station. */
    SyncStations  /**< Triggers a refresh of the station database. */
};

/** @enum StopResult
 * @brief Outcomes for task termination requests.
 */
enum class StopResult : uint8_t {
    Ok,            /**< Task stopped successfully. */
    InvalidHandle, /**< Provided handle does not point to an active task. */
    Timeout        /**< Task failed to exit within the requested window. */
};

/** @enum StepAction
 * @brief Instructions returned by a task's step function to the runner.
 */
enum class StepAction : uint8_t {
    Continue, /**< Execute the next iteration immediately. */
    Sleep,    /**< Wait for a specified duration before the next iteration. */
    Done,     /**< Task has completed its work gracefully. */
    Error     /**< Task encountered an unrecoverable failure. */
};

/** @enum Button
 * @brief Logical identifiers for physical hardware buttons.
 */
enum class Button : uint8_t {
    PlayStop = 0U, /**< Primary action button. */
    Next,          /**< Selection increment button. */
    Previous       /**< Selection decrement button. */
};

/** @enum PlaybackStatus
 * @brief Current state of the audio engine.
 */
enum class PlaybackStatus : uint8_t {
    Idle = 0U, /**< No station selected or active. */
    Buffering, /**< Network data is being cached. */
    Playing,   /**< Audio is currently being output. */
    Stopped,   /**< Playback is paused or terminated. */
    Error      /**< Pipeline failure (e.g., source not found). */
};

/** @enum GpioInputType
 * @brief Classification of physical interaction events.
 */
enum class GpioInputType : uint8_t {
    ButtonPressed = 0U, /**< Initial contact made. */
    ButtonReleased,     /**< Contact broken. */
    ButtonLongPressed,  /**< Held beyond a specific threshold. */
};

/** @enum Icon
 * @brief Identifiers for small UI status glyphs.
 */
enum class Icon : uint8_t {
    WifiOff = 0U,
    Wifi1 = 1U,
    Wifi2 = 2U,
    Wifi3 = 3U,
    Play = 4U,
    Buffering = 5U,
    Stop = 6U,
    BatteryLow = 7U,
    BatteryMid = 8U,
    BatteryFull = 9U,
    Volume0 = 10U,
    Volume1 = 11U,
    Volume2 = 12U,
    Volume3 = 13U,
    Volume4 = 14U,
    Volume5 = 15U,
    _MAX = 31U
};
/** @} */

/** @name Data Structures
 * Aggregated data types for hardware features and application state.
 * @{ */

/** @struct MicFeatures
 * @brief Processed metrics from the microphone input.
 */
struct MicFeatures {
    int32_t energy = 0;   /**< Total signal energy (volume level). */
    int32_t p2p = 0;      /**< Peak-to-peak amplitude. */
    int32_t peakDiff = 0; /**< Difference between successive peaks. */
    int32_t minRaw = 0;   /**< Minimum raw sample value in the window. */
    int32_t maxRaw = 0;   /**< Maximum raw sample value in the window. */
};

/** @struct Rect
 * @brief Simple 2D geometry definition.
 */
struct Rect {
    uint8_t x; /**< Horizontal origin. */
    uint8_t y; /**< Vertical origin. */
    uint8_t w; /**< Width in pixels. */
    uint8_t h; /**< Height in pixels. */
};

/** @struct StationData
 * @brief Metadata for a streamable radio station.
 */
struct StationData {
    std::string id;   /**< Unique identifier for the station. */
    std::string name; /**< Human-readable display name. */
    std::string url;  /**< Source stream URL. */
};

/** @struct ManifestData
 * @brief Versioning and system identity information.
 */
struct ManifestData {
    std::string version; /**< Firmware or configuration version string. */
};

/** @struct GpioInputData
 * @brief Detailed event data for a hardware input change.
 */
struct GpioInputData {
    GpioInputType type;   /**< The nature of the interaction. */
    uint32_t buttonId;    /**< Which button triggered the event. */
    uint64_t timestampMs; /**< System uptime when the event occurred. */
};

/** @struct StepResult
 * @brief Control structure returned by a task's unit of work.
 */
struct StepResult {
    StepAction action{StepAction::Continue}; /**< Desired runner behavior. */
    uint32_t sleepMs{0U};                    /**< Delay duration if action is Sleep. */
};

/** @struct TaskHandle
 * @brief Opaque identifier for a managed background task.
 */
struct TaskHandle {
    static constexpr uint16_t InvalidSlot = 0xFFFF;
    uint16_t slot{InvalidSlot}; /**< Index in the TaskRunner slot array. */
    uint16_t runId{0U};         /**< Unique generation ID for the current run. */

    /** @brief Checks if the handle points to a valid task slot. */
    constexpr bool isValid() const {
        return slot != InvalidSlot && runId != 0U;
    }

    /** @brief Invalidates the handle. */
    constexpr void reset() {
        slot = InvalidSlot;
        runId = 0U;
    }
};

/** @struct TaskParams
 * @brief Configuration required to launch a new task.
 */
struct TaskParams {
    const char* name{"Task"}; /**< Descriptive name for the OS. */
    uint16_t priority{1U};    /**< RTOS priority level. */
    uint16_t core{0U};        /**< CPU core affinity (for ESP32). */
};

/** @struct Mp3FrameInfo
 * @brief Detailed properties of a decoded MP3 audio frame.
 */
struct Mp3FrameInfo {
    int frameBytes = 0;   /**< Bytes consumed from source; 0 if incomplete. */
    int hz = 0;           /**< Sample rate in Hertz. */
    int channels = 0;     /**< Number of audio channels (1 or 2). */
    int samplesPerCh = 0; /**< Samples produced per channel. */
};

/** @struct WifiState
 * @brief Current connectivity status and signal strength.
 */
struct WifiState {
    bool isConnected; /**< True if the radio is associated with an AP. */
    int8_t rssi;      /**< Received Signal Strength Indicator in dBm. */
};

/** @struct WifiCredentials
 * @brief Network access information.
 */
struct WifiCredentials {
    std::string ssid;     /**< Service Set Identifier. */
    std::string password; /**< WPA2/WPA3 Pre-shared key. */
};

/** @struct ProvisioningPortalConfig
 * @brief Configuration for the device-hosted setup Access Point.
 */
struct ProvisioningPortalConfig {
    std::string apSsid;     /**< SSID to broadcast in AP mode. */
    std::string apPassword; /**< Password for the local setup network. */
    uint8_t channel;        /**< WiFi channel (1-11). */
    uint8_t maxConnections; /**< Maximum simultaneous clients allowed. */
};
/** @} */

struct IStopToken;

/** @name Callback Types
 * Function signatures for event notification and task execution.
 * @{ */
using PlaybackStatusCallback = std::function<void(const PlaybackStatus&)>;
using GpioInputDataCallback = std::function<void(const GpioInputData&)>;
using WifiStateCallback = std::function<void(const WifiState&)>;
using CredentialsCallback = std::function<void(const WifiCredentials&)>;

/**
 * @brief Standard task loop function signature.
 * @param user Opaque pointer to task-specific data.
 * @param token Token used to check for stop requests or perform sleeps.
 * @return Instructions for the runner on how to proceed.
 */
using StepFn = StepResult (*)(void* user, IStopToken& token);
/** @} */

}  // namespace common
