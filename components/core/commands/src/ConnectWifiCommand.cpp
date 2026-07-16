#include "ConnectWifiCommand.hpp"

#include <esp_log.h>

#include "IEventQueue.hpp"
#include "IWifiService.hpp"
#include "Overloaded.hpp"

namespace core::commands {
namespace {
constexpr const char* Tag = "ConnectWifiCommand";
}  // namespace

ConnectWifiCommand::ConnectWifiCommand(services::IWifiService& wifiService,
                                       common::IEventQueue& uiEventQueue)
    : mWifiService(wifiService),
      mUiEventQueue(uiEventQueue),
      mProvisioningPortalStarted(false),
      mResult(std::nullopt) {}

void ConnectWifiCommand::handle(const common::AppEvent& e) {
    if (mResult.has_value()) {
        return;
    }

    std::visit(
        common::Overloaded{
            [this](const common::SystemInitedEvent&) { connect(); },
            [this](const common::WifiCredsReceivedEvent& w) { onCredsReceived(); },
            [this](const common::WifiStateChangedEvent& w) { onWifiStateChanged(w.isConnected); },
            [](const auto&) {}},
        e);
}

bool ConnectWifiCommand::isFinished() {
    return mResult.has_value();
}

common::CommandType ConnectWifiCommand::getCmdType() {
    return common::CommandType::ConnectWifi;
}

std::optional<bool> ConnectWifiCommand::getResult() {
    return mResult;
}

void ConnectWifiCommand::connect() {
    if (!mWifiService.connect()) {
        ESP_LOGE(Tag, "WiFi connection failed, switching to wifi prov mode");

        if (!mProvisioningPortalStarted) {
            const bool res = mWifiService.startProvisioningPortal();
            if (!res) {
                ESP_LOGE(Tag, "Failed to start provisioning portal. Finish with error");
                mResult = false;
                return;
            }

            mProvisioningPortalStarted = true;
            mUiEventQueue.post(common::SwitchToWifiProvScreenEvent{});
        }
    }
}

void ConnectWifiCommand::onWifiStateChanged(const bool isConnected) {
    if (!isConnected) {
        ESP_LOGE(Tag, "WiFi connection failed");

        if (!mProvisioningPortalStarted) {
            if (!mWifiService.startProvisioningPortal()) {
                ESP_LOGE(Tag, "Failed to start provisioning portal. Finish with error");
                mResult = false;
                return;
            }

            mProvisioningPortalStarted = true;
            mUiEventQueue.post(common::SwitchToWifiProvScreenEvent{});
        }
    } else {
        mResult = true;
        mUiEventQueue.post(common::SwitchToMainScreenEvent{});
    }
}

void ConnectWifiCommand::onCredsReceived() {
    mWifiService.disconnect();
    connect();
}

}  // namespace core::commands
