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

struct TemperatureUpdateEvent {
    float temperature;
};

struct SystemReadyEvent {
    bool showSplashScreen;
};

struct UiRenderEvent {
    RenderType renderType;
    uint32_t selectedStationIndex{0U};
};

struct WifiStateChangedEvent {
    bool isConnected;
};

using AppEvent =
    std::variant<ButtonPressedEvent, PlaybackStatusChangedEvent, TemperatureUpdateEvent,
                 SystemReadyEvent, UiRenderEvent, WifiStateChangedEvent>;

}  // namespace common
