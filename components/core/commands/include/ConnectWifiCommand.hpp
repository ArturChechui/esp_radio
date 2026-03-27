#pragma once

#include <optional>
#include <string>

#include "Events.hpp"
#include "ICommand.hpp"
#include "Types.hpp"

namespace common {
class IEventQueue;
}

namespace services {
class IWifiService;
class IStationRepository;
}  // namespace services

namespace core::commands {
class ConnectWifiCommand : public ICommand {
   public:
    ConnectWifiCommand(services::IWifiService& wifiService, common::IEventQueue& uiEventQueue);
    ~ConnectWifiCommand() override = default;

    void handle(const common::AppEvent& e) override;
    bool isFinished() override;

   private:
    void connect();
    void onWifiStateChanged(const bool isConnected);
    void onCredsReceived();

   private:
    services::IWifiService& mWifiService;
    common::IEventQueue& mUiEventQueue;

    bool mProvisioningPortalStarted;
    bool mFinished;
};

}  // namespace core::commands
