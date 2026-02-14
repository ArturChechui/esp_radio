#pragma once
#include <string>

#include "Events.hpp"
#include "Overloaded.hpp"
#include "Types.hpp"

namespace common {
inline std::string dump(const Button& b) {
    switch (b) {
        case Button::PlayStop:
            return "PlayStop";
        case Button::Up:
            return "Up";
        case Button::Down:
            return "Down";
        default:
            return "Unknown";
    }
}

inline std::string dump(const PlaybackStatus& p) {
    switch (p) {
        case PlaybackStatus::Playing:
            return "Playing";
        case PlaybackStatus::Buffering:
            return "Buffering";
        case PlaybackStatus::Stopped:
            return "Stopped";
        case PlaybackStatus::Idle:
            return "Idle";
        case PlaybackStatus::Error:
            return "Error";
        default:
            return "Unknown";
    }
}

inline std::string dump(const Icon& i) {
    switch (i) {
        case Icon::WifiOff:
            return "WifiOff";
        case Icon::Wifi1:
            return "Wifi1";
        case Icon::Wifi2:
            return "Wifi2";
        case Icon::Wifi3:
            return "Wifi3";
        case Icon::Play:
            return "Play";
        case Icon::Buffering:
            return "Buffering";
        case Icon::Stop:
            return "Stop";
        case Icon::BatteryLow:
            return "BatteryLow";
        case Icon::BatteryMid:
            return "BatteryMid";
        case Icon::BatteryFull:
            return "BatteryFull";
        case Icon::Speaker:
            return "Speaker";
        default:
            return "Unknown";
    }
}

inline std::string dump(const StationData& s) {
    std::string out;
    out.reserve(8 + s.name.size() + 1);
    out += "{name=";
    out += s.name;
    out += "}";
    return out;
}

inline std::string dump(const TaskHandle& t) {
    std::string out;
    out.reserve(24);  // enough for "{runId=65535,slot=65535}"
    out += "{runId=";
    out += std::to_string(t.runId);
    out += ",slot=";
    out += std::to_string(t.slot);
    out += "}";
    return out;
}

inline std::string dump(const common::AppEvent& ev) {
    return std::visit(
        Overloaded{[](const ButtonPressedEvent& e) {
                       return "ButtonPressedEvent{b=" + dump(e.button) + "}";
                   },
                   [](const PlaybackStatusChangedEvent& e) {
                       return "PlaybackStatusChangedEvent{s=" + dump(e.status) + "}";
                   },
                   [](const TempHumidUpdateEvent& e) {
                       return "TempHumidUpdateEvent{t=" + std::to_string(e.temperature) +
                              ", h=" + std::to_string(e.humidity) + "}";
                   },
                   [](const SystemReadyEvent& e) {
                       return std::string("SystemReadyEvent{s=") +
                              (e.showSplashScreen ? "true}" : "false}");
                   },
                   [](const CurrentStationChangedEvent&) {
                       return std::string{"CurrentStationChangedEvent{}"};
                   },
                   [](const WifiStateChangedEvent& e) {
                       return std::string("WifiStateChangedEvent{c=") +
                              (e.isConnected ? "true, b=" : "false, b=") + std::to_string(e.bars) +
                              "}";
                   },
                   [](const auto&) { return std::string{"<unknown event>"}; }},
        ev);
}

}  // namespace common
