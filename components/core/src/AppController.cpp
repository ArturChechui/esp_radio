#include "AppController.hpp"

#include "IEventQueue.hpp"
#include "IPlayerService.hpp"
#include "IStationRepository.hpp"
#include "PlayStopSkipCommand.hpp"

// IDF
#include <esp_log.h>

namespace core {
namespace {
constexpr const char* Tag = "AppController";
}  // namespace

AppController::AppController(services::IPlayerService& playerService,
                             services::IStationRepository& stationRepository,
                             common::IEventQueue& uiEventQueue)
    : mPlayerService(playerService),
      mStationRepository(stationRepository),
      mUiEventQueue(uiEventQueue),
      mCurrentCmd(nullptr) {}

bool AppController::init() {
    ESP_LOGI(Tag, "Initializing AppController");

    // TODO: any initialization logic

    return true;
}

void AppController::onEvent(const common::AppEvent& event) {
    // try to handle it by the current cmd
    if (mCurrentCmd && !mCurrentCmd->isFinished()) {
        if (mCurrentCmd->handle(event)) {
            // handled
            if (mCurrentCmd->isFinished()) {
                // command is finished, so can be destroyed
                mCurrentCmd.reset();
            }
            return;
        }
    }

    if (std::holds_alternative<common::SystemReadyEvent>(event)) {
        handleSystemReadyEvent(std::get<common::SystemReadyEvent>(event));
    } else if (std::holds_alternative<common::ButtonPressedEvent>(event)) {
        const auto& ev = std::get<common::ButtonPressedEvent>(event);

        // TODO: handle the case when play is active and stop is being pressed or vice versa
        mCurrentCmd = std::make_unique<commands::PlayStopSkipCommand>(
            mPlayerService, mStationRepository, ev.button);
        if (mCurrentCmd && mCurrentCmd->handle(event) && mCurrentCmd->isFinished()) {
            mCurrentCmd.reset();
        }
    } else if (std::holds_alternative<common::TemperatureUpdateEvent>(event)) {
        handleTemperatureUpdateEvent(std::get<common::TemperatureUpdateEvent>(event));
    } else if (std::holds_alternative<common::WifiStateChangedEvent>(event)) {
        ESP_LOGI(Tag, "Wifi is connected. Work can be started");
    } else if (std::holds_alternative<common::PlaybackStatusChangedEvent>(event)) {
        ESP_LOGI(Tag, "PlaybackStatusChangedEvent %d",
                 static_cast<int>(std::get<common::PlaybackStatusChangedEvent>(event).status));
    } else {
        ESP_LOGW(Tag, "Unhandled event type");
    }
}

void AppController::handleSystemReadyEvent(const common::SystemReadyEvent& event) {
    ESP_LOGI(Tag, "System ready event received (showSplashScreen=%d)", event.showSplashScreen);

    // TODO: booting + restore last mode cmd

    // mUiEventQueue.post(common::UiRenderEvent{.renderType = common::RenderType::Boot});

    // Temporarily show stations screen after boot before implementing full logic
    mUiEventQueue.post(common::UiRenderEvent{.renderType = common::RenderType::Stations});
}

void AppController::handleButtonPressedEvent(const common::ButtonPressedEvent& event) {
    ESP_LOGI(Tag, "Button pressed: %d", static_cast<int>(event.button));
}

void AppController::handleTemperatureUpdateEvent(const common::TemperatureUpdateEvent& event) {
    ESP_LOGI(Tag, "Temperature: %.1f°C", event.temperature);
}

}  // namespace core
