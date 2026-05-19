/**
 * @file UiService.hpp
 * @brief Implementation of the UI management service.
 *
 * This file contains the UiService class, which handles the rendering logic
 * for the device's display, including different modes like main playback,
 * provisioning, and synchronization.
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "IDisplay.hpp"
#include "IEventHandler.hpp"
#include "IStationRepository.hpp"
#include "Types.hpp"

namespace services {

/**
 * @brief Defines the available operational modes for the User Interface.
 */
enum class UiMode {
    Booting,       /**< Initial system startup screen. */
    Main,          /**< Primary playback and status screen. */
    WifiProv,      /**< Wi-Fi provisioning/captive portal info screen. */
    SyncInProgress /**< Station database synchronization screen. */
};

/**
 * @class UiService
 * @brief Service responsible for managing the display and rendering UI components.
 *
 * UiService acts as an event handler that reacts to system state changes by
 * updating a local framebuffer. It supports both full-screen redraws and
 * optimized partial updates using "dirty" flags to track changed components.
 */
class UiService : public common::IEventHandler {
   public:
    /**
     * @brief Constructs a UiService with display and station repository dependencies.
     * @param display Reference to the hardware display adapter.
     * @param stationRepo Reference to the repository for retrieving current station info.
     */
    UiService(adapters::IDisplay& display, IStationRepository& stationRepo);

    /**
     * @brief Initializes the UI service and allocates the internal framebuffer.
     * @return true if initialization was successful.
     */
    bool init();

    /**
     * @brief Processes application events to trigger UI updates.
     * @param event The application event to handle.
     */
    void onEvent(const common::AppEvent& event) override;

    // TODO: make ifdef for tests
    /**
     * @brief Retrieves a reference to the internal UI framebuffer.
     * @return A constant reference to the raw pixel data vector.
     */
    const std::vector<uint8_t>& getFramebuffer() const {
        return mFramebuffer;
    }

   private:
    /** @brief Renders the complete layout for the Main operation screen. */
    void showFullMainScreen();

    /** @brief Renders the Wi-Fi provisioning info screen. */
    void showWifiProvisioningScreen();

    /** @brief Renders the synchronization status screen. */
    void showSyncInProgressScreen();

    /**
     * @brief Updates the status line text (e.g., Temperature/Humidity).
     * @param doFlush If true, the modified region is pushed to the display hardware.
     */
    void updateStatusText(const bool doFlush = true);

    /** @brief Updates the Wi-Fi signal strength icon. */
    void updateWifiIcon(const bool doFlush = true);

    /** @brief Updates the battery level icon. */
    void updateBatteryIcon(const bool doFlush = true);

    /** @brief Updates the volume level indicator icon. */
    void updateVolumeIcon(const bool doFlush = true);

    /** @brief Updates the playback state icon (Play/Stop/Pause). */
    void updatePlaybackIcon(const bool doFlush = true);

    /** @brief Updates the current radio station name display. */
    void updateStationName(const bool doFlush = true);

    /** @brief Clears a specific rectangular region in the framebuffer. */
    void clearRect(const common::Rect r);

    /** @brief Copies bitmap data into a specific region of the framebuffer. */
    void blitRect(const common::Rect r, const uint8_t* data);

    /** @brief Pushes a modified framebuffer region to the display hardware. */
    void flushRect(const common::Rect r);

   private:
    adapters::IDisplay& mDisplay;     /**< Reference to hardware display driver. */
    IStationRepository& mStationRepo; /**< Reference to station data repository. */

    std::vector<uint8_t> mFramebuffer; /**< Local buffer containing current screen pixels. */
    std::vector<uint8_t> mTxBuf;       /**< Intermediate buffer for hardware transmissions. */

    int8_t mTemperatureC;   /**< Cached temperature value. */
    uint8_t mHumidityPct;   /**< Cached humidity value. */
    common::Icon mWifi;     /**< Current state of the Wi-Fi icon. */
    common::Icon mBattery;  /**< Current state of the battery icon. */
    common::Icon mPlayback; /**< Current state of the playback icon. */
    common::Icon mVolume;   /**< Current state of the volume icon. */

    bool mFullFlushPending; /**< Flag: Indicates a full screen refresh is required. */
    UiMode mMode;           /**< Active UI operational mode. */
    bool mStationDirty;     /**< Flag: Station name requires redrawing. */
    bool mStatusDirty;      /**< Flag: Status text (temp/hum) requires redrawing. */
    bool mWifiDirty;        /**< Flag: Wi-Fi icon requires redrawing. */
    bool mBatteryDirty;     /**< Flag: Battery icon requires redrawing. */
    bool mPlaybackDirty;    /**< Flag: Playback icon requires redrawing. */
    bool mVolumeDirty;      /**< Flag: Volume icon requires redrawing. */

    bool mClapFeatureEnabled; /**< Cached state of the clap feature; used to determine if the UI
                                 indicator line should be rendered. */
};

}  // namespace services
