#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "Types.hpp"

namespace common {
struct ButtonPressedEvent {
    Button button;
};

struct PlaybackStatusChangedEvent {
    PlaybackStatus status;
};

struct TempHumidUpdateEvent {
    int8_t temperature;
    uint8_t humidity;
};

struct SystemReadyEvent {
    bool showSplashScreen;
};

struct CurrentStationChangedEvent {};

struct WifiStateChangedEvent {
    bool isConnected;
    uint8_t bars;
};

struct VolumeChangedEvent {
    uint8_t volume;
};

using AppEvent = std::variant<ButtonPressedEvent, PlaybackStatusChangedEvent, TempHumidUpdateEvent,
                              SystemReadyEvent, CurrentStationChangedEvent, WifiStateChangedEvent,
                              VolumeChangedEvent>;

}  // namespace common
