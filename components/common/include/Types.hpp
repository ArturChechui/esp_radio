#pragma once

#include <stdint.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace common {
enum class CommandType : uint8_t { PlayStopSkip, RestoreLastMode, UpdateStations };

struct TaskHandle {
    static constexpr uint16_t InvalidSlot = 0xFFFF;
    uint16_t slot{InvalidSlot};
    uint16_t runId{0U};

    constexpr bool isValid() const {
        return slot != InvalidSlot && runId != 0U;
    }
};

enum class StopResult : uint8_t { Ok, InvalidHandle, Timeout };

struct TaskParams {
    const char* name{"Task"};
    uint16_t priority{1U};
    uint16_t core{0U};
};

enum class StepAction : uint8_t { Continue, Sleep, Done, Error };

struct StepResult {
    StepAction action{StepAction::Continue};
    uint32_t sleepMs{0U};  // used when action==Sleep
};

struct IStopToken;
using StepFn = StepResult (*)(void* user, IStopToken& token);

struct Mp3FrameInfo {
    int frameBytes = 0;  // 0 => no frame decoded (need more / resync)
    int hz = 0;
    int channels = 0;      // 1 or 2
    int samplesPerCh = 0;  // >0 when decoded
};

// Enumerations
enum class Button : uint8_t { PlayStop = 0U, Up, Down };
enum class PlaybackStatus : uint8_t { Idle = 0U, Buffering, Playing, Paused, Stopped, Error };
enum class RenderType : uint8_t { Stations = 0U, Status, Boot };
enum class GpioInputType : uint8_t {
    ButtonPressed = 0U,
    ButtonReleased,
    ButtonLongPressed,
};
enum class Icon : uint8_t {
    WifiOff = 0U,
    Wifi1 = 1U,
    Wifi2 = 2U,
    Wifi3 = 3U,
    Play = 4U,
    Pause = 5U,
    Stop = 6U,
    Speaker = 7U,
    BatteryEmpty = 8U,
    BatteryLow = 9U,
    BatteryMid = 10U,
    BatteryFull = 11U,
    _MAX = 31U
};
enum class UiStatusKind : uint8_t {
    Booting,
    WifiConnecting,
    WifiConnected,
    WifiError,
    Playing,
    Stopped,
    Error
};

// Structures
struct StationData {
    // TODO: std::string_view?
    std::string id;    // e.g., "radio1_aac_h"
    std::string name;  // e.g., "Radio 1 (AAC High)"
    std::string url;   // streaming URL
};

struct UiStatus {
    UiStatusKind kind{UiStatusKind::Booting};
    std::string line1;
    std::string line2;
};

struct GpioInputData {
    GpioInputType type;
    uint32_t buttonId;
    uint64_t timestampMs;
};

// Callback types
using PlaybackStatusCallback = std::function<void(const PlaybackStatus&)>;
using GpioInputDataCallback = std::function<void(const GpioInputData&)>;

}  // namespace common
