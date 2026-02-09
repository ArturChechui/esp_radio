#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "Events.hpp"
#include "IEventHandler.hpp"

namespace adapters {
class IDisplay;
}  // namespace adapters

namespace services {
class IStationRepository;

class UiService : public common::IEventHandler {
   public:
    explicit UiService(adapters::IDisplay &display, IStationRepository &stationRepo);
    bool init();
    void onEvent(const common::AppEvent &event) override;

#ifdef UNIT_TESTS
    const std::vector<uint8_t> &getFramebuffer() {
        return mFramebuffer;
    }
#endif
   private:
    void renderBoot();
    void renderStatus();
    void renderStations();

    void clearFramebuffer();
    void flushFramebuffer();

    void drawText(const uint8_t &x, const uint8_t &y, const std::string_view &txt);
    void drawChar(const uint8_t &x, const uint8_t &y, const char &c);

    adapters::IDisplay &mDisplay;
    IStationRepository &mStationRepo;
    std::vector<uint8_t> mFramebuffer;
};

}  // namespace services
