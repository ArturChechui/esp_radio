#pragma once

#include <stdint.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace common {
// Enumerations
enum class CommandType : uint8_t { PlayStopSkip, RestoreLastMode, UpdateStations };
enum class StopResult : uint8_t { Ok, InvalidHandle, Timeout };
enum class StepAction : uint8_t { Continue, Sleep, Done, Error };
enum class Button : uint8_t { PlayStop = 0U, Next, Previous };
enum class PlaybackStatus : uint8_t { Idle = 0U, Buffering, Playing, Stopped, Error };
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

// Structures
struct Rect {
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
};

struct StationData {
    // TODO: std::string_view?
    std::string id;
    std::string name;
    std::string url;
};

struct GpioInputData {
    GpioInputType type;
    uint32_t buttonId;
    uint64_t timestampMs;
};

struct StepResult {
    StepAction action{StepAction::Continue};
    uint32_t sleepMs{0U};  // used when action==Sleep
};

struct TaskHandle {
    static constexpr uint16_t InvalidSlot = 0xFFFF;
    uint16_t slot{InvalidSlot};
    uint16_t runId{0U};

    constexpr bool isValid() const {
        return slot != InvalidSlot && runId != 0U;
    }

    constexpr void reset() {
        slot = InvalidSlot;
        runId = 0U;
    }
};

struct TaskParams {
    const char* name{"Task"};
    uint16_t priority{1U};
    uint16_t core{0U};
};
struct Mp3FrameInfo {
    int frameBytes = 0;  // 0 => no frame decoded (need more / resync)
    int hz = 0;
    int channels = 0;      // 1 or 2
    int samplesPerCh = 0;  // >0 when decoded
};

struct WifiData {
    bool isConnected;
    int8_t rssi;
};

struct IStopToken;

// Callback types
using PlaybackStatusCallback = std::function<void(const PlaybackStatus&)>;
using GpioInputDataCallback = std::function<void(const GpioInputData&)>;
using WifiStateCallback = std::function<void(const WifiData&)>;
using StepFn = StepResult (*)(void* user, IStopToken& token);
}  // namespace common
