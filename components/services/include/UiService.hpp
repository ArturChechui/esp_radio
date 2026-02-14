#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "IDisplay.hpp"
#include "IEventHandler.hpp"
#include "IStationRepository.hpp"
#include "Types.hpp"

namespace services {

class UiService : public common::IEventHandler {
   public:
    UiService(adapters::IDisplay& display, IStationRepository& stationRepo);

    bool init();

    void onEvent(const common::AppEvent& event) override;

    // TODO: make ifdef for tests
    const std::vector<uint8_t>& getFramebuffer() const {
        return mFramebuffer;
    }

   private:
    // no I2C, only draws into mFramebuffer
    void renderFullToFramebuffer();

    // Partial updates
    void updateStatusText(const bool doFlush = true);
    void updateWifiIcon(const bool doFlush = true);
    void updateBatteryIcon(const bool doFlush = true);
    void updatePlaybackIcon(const bool doFlush = true);
    void updateStationName(const bool doFlush = true);

    // Low-level framebuffer ops
    void clearRect(const common::Rect r);
    void blitRect(const common::Rect r, const uint8_t* data);
    void flushRect(const common::Rect r);

   private:
    adapters::IDisplay& mDisplay;
    IStationRepository& mStationRepo;

    std::vector<uint8_t> mFramebuffer;
    std::vector<uint8_t> mTxBuf;

    // UI state
    uint8_t mTemperatureC;
    uint8_t mHumidityPct;
    common::Icon mWifi;
    common::Icon mBattery;
    common::Icon mPlayback;

    bool mFullFlushPending;
};

}  // namespace services
