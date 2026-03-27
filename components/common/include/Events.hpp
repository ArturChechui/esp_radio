#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "Types.hpp"

namespace common {
struct ButtonPressedEvent {
    Button button;
};

struct ButtonLongPressedEvent {
    Button button;
};

struct PlaybackStatusChangedEvent {
    PlaybackStatus status;
};

struct TempHumidUpdateEvent {
    int8_t temperature;
    uint8_t humidity;
};

struct LightLevelUpdateEvent {
    uint16_t lux;
};

struct BatteryLevelUpdateEvent {
    uint16_t millivolts;
    uint8_t percent;
};

struct SystemInitedEvent {};

struct CurrentStationChangedEvent {};

struct WifiStateChangedEvent {
    bool isConnected;
    uint8_t bars;
};

struct VolumeChangedEvent {
    uint8_t volume;
};

struct SwitchToMainScreenEvent {};
struct SwitchToWifiProvScreenEvent {};
struct SwitchToSyncInProgressScreenEvent {};
struct WifiCredsReceivedEvent {};

using AppEvent =
    std::variant<ButtonPressedEvent, ButtonLongPressedEvent, PlaybackStatusChangedEvent,
                 TempHumidUpdateEvent, LightLevelUpdateEvent, BatteryLevelUpdateEvent,
                 SystemInitedEvent, CurrentStationChangedEvent, WifiStateChangedEvent,
                 VolumeChangedEvent, SwitchToMainScreenEvent, SwitchToWifiProvScreenEvent,
                 SwitchToSyncInProgressScreenEvent, WifiCredsReceivedEvent>;

}  // namespace common
