/**
 * @file Events.hpp
 * @brief Definition of application-level events and the core event variant.
 *
 * This file contains the structures that represent various system events,
 * such as hardware interactions, sensor updates, and state changes.
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <variant>

#include "Types.hpp"

namespace common {

/**
 * @struct ButtonPressedEvent
 * @brief Triggered when a physical button is pressed and released quickly.
 */
struct ButtonPressedEvent {
    Button button; /**< The specific button that was pressed. */
};

/**
 * @struct ButtonLongPressedEvent
 * @brief Triggered when a physical button is held down for an extended period.
 */
struct ButtonLongPressedEvent {
    Button button; /**< The specific button that was long-pressed. */
};

/**
 * @struct PlaybackStatusChangedEvent
 * @brief Triggered when the audio engine transitions between states (e.g., Playing to Stopped).
 */
struct PlaybackStatusChangedEvent {
    PlaybackStatus status; /**< The new playback status. */
};

/**
 * @struct TempHumidUpdateEvent
 * @brief Triggered when new environmental data is read from the AHT20 sensor.
 */
struct TempHumidUpdateEvent {
    int8_t temperature; /**< Current temperature in degrees Celsius. */
    uint8_t humidity;   /**< Current relative humidity percentage (0-100). */
};

/**
 * @struct LightLevelUpdateEvent
 * @brief Triggered when new ambient light data is read from the BH1750 sensor.
 */
struct LightLevelUpdateEvent {
    uint16_t lux; /**< Ambient light intensity in Lux. */
};

/**
 * @struct BatteryLevelUpdateEvent
 * @brief Triggered when the system battery voltage and percentage are updated.
 */
struct BatteryLevelUpdateEvent {
    uint16_t millivolts; /**< Current battery potential in mV. */
    uint8_t percent;     /**< Calculated battery charge percentage (0-100). */
};

/**
 * @struct SystemInitedEvent
 * @brief Broadcasted once all core services have successfully finished initialization.
 */
struct SystemInitedEvent {};

/**
 * @struct ClapFeatureStateChangedEvent
 * @brief Event dispatched when the master state of the clap detection feature changes.
 */
struct ClapFeatureStateChangedEvent {
    bool isEnabled; /**< True if the clap feature is now globally enabled, false if disabled. */
};

/**
 * @struct CurrentStationChangedEvent
 * @brief Triggered when the active radio station index is modified in the repository.
 */
struct CurrentStationChangedEvent {};

/**
 * @struct WifiStateChangedEvent
 * @brief Triggered when the Wi-Fi connection status or signal strength changes.
 */
struct WifiStateChangedEvent {
    bool isConnected; /**< True if the device is connected to an Access Point. */
    uint8_t bars;     /**< Signal strength represented as 0-3 bars. */
};

/**
 * @struct VolumeChangedEvent
 * @brief Triggered when the system output volume is adjusted.
 */
struct VolumeChangedEvent {
    uint8_t volume; /**< The new volume level (typically 0-255). */
};

/** @struct SwitchToMainScreenEvent @brief Request to transition the UI to the primary playback
 * screen. */
struct SwitchToMainScreenEvent {};

/** @struct SwitchToWifiProvScreenEvent @brief Request to transition the UI to the Wi-Fi
 * provisioning status screen. */
struct SwitchToWifiProvScreenEvent {};

/** @struct SwitchToSyncInProgressScreenEvent @brief Request to transition the UI to the station
 * synchronization screen. */
struct SwitchToSyncInProgressScreenEvent {};

/** @struct WifiCredsReceivedEvent @brief Triggered when new Wi-Fi credentials have been received
 * via the captive portal. */
struct WifiCredsReceivedEvent {};

/**
 * @brief A type-safe union (variant) representing any application-level event.
 *
 * AppEvent is the primary message type sent through the system's event queues.
 * Receivers typically use std::visit with a visitor pattern to handle specific event types.
 */
using AppEvent = std::variant<
    ButtonPressedEvent, ButtonLongPressedEvent, PlaybackStatusChangedEvent, TempHumidUpdateEvent,
    LightLevelUpdateEvent, BatteryLevelUpdateEvent, SystemInitedEvent, ClapFeatureStateChangedEvent,
    CurrentStationChangedEvent, WifiStateChangedEvent, VolumeChangedEvent, SwitchToMainScreenEvent,
    SwitchToWifiProvScreenEvent, SwitchToSyncInProgressScreenEvent, WifiCredsReceivedEvent>;

/**
 * @brief Compile-time validation of the AppEvent variant.
 *
 * This ensures that no non-trivially copyable types (like std::string or std::vector)
 * are added to AppEvent. If someone attempts to add one, the compilation will fail
 * immediately with a clear error message.
 */
static_assert(std::is_trivially_copyable_v<AppEvent>,
              "ERROR: AppEvent is not trivially copyable! This will cause memory corruption "
              "in FreeRTOS queues. Ensure all variant types are POD/trivially copyable (e.g., "
              "use fixed-size char arrays instead of std::string).");

}  // namespace common
