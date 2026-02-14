#pragma once

#include <memory>

#include "Events.hpp"
#include "ICommand.hpp"
#include "IEventHandler.hpp"

namespace common {
class IEventQueue;
}  // namespace common

namespace services {
class IPlayerService;
class IStationRepository;
}  // namespace services

namespace commands {
class ICommand;
}  // namespace commands

namespace core {
class IUiEventSink;

class AppController : public common::IEventHandler {
   public:
    AppController(services::IPlayerService& playerService,
                  services::IStationRepository& stationRepository,
                  common::IEventQueue& uiEventQueue);
    bool init();

    // IEventHandler
    void onEvent(const common::AppEvent& event) override;

   private:
    void handleSystemReadyEvent(const common::SystemReadyEvent& event);
    void handleButtonPressedEvent(const common::ButtonPressedEvent& event);
    void handleTempHumidUpdateEvent(const common::TempHumidUpdateEvent& event);

    services::IPlayerService& mPlayerService;
    services::IStationRepository& mStationRepository;
    common::IEventQueue& mUiEventQueue;
    std::unique_ptr<commands::ICommand> mCurrentCmd;
};

}  // namespace core
