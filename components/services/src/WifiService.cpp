#include "WifiService.hpp"

#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ITaskRunner.hpp"

namespace services {
namespace {
constexpr const char* Tag = "WifiService";
constexpr int SignalTaskPriority = 5;
constexpr int SignalTaskCore = 0;
constexpr uint32_t SignalTaskStackWords = 3456U;
constexpr uint32_t TimeoutToExitTasksMs = 7000U;

static uint8_t rssiToBars(int8_t rssiDbm) {
    if (rssiDbm >= -55) {
        return 3U;
    } else if (rssiDbm >= -70) {
        return 2U;
    } else if (rssiDbm >= -85) {
        return 1U;
    } else {
        return 0U;
    }
}
}  // namespace

WifiService::WifiService(std::shared_ptr<adapters::IWifiClient> adapter,
                         common::ITaskRunner& taskRunner)
    : mWifiAdapter(adapter),
      mCoreEventQueue(nullptr),
      mLastBars(0U),
      mSignalTaskHandle(),
      mTaskRunner(taskRunner) {}

bool WifiService::connect(const std::string& ssid, const std::string& password,
                          common::IEventQueue& coreEventQueue, uint32_t timeoutMs) {
    if (!mWifiAdapter) {
        ESP_LOGE(Tag, "WiFi adapter not set");
        return false;
    }

    mCoreEventQueue = &coreEventQueue;

    ESP_LOGI(Tag, "Connecting to WiFi \"%s\"...", ssid.c_str());

    if (!mWifiAdapter->init(ssid, password)) {
        ESP_LOGE(Tag, "Failed to initialize WiFi adapter");
        return false;
    }

    mWifiAdapter->setStateCallback(
        [this](const common::WifiData& data) { this->onWifiStateChanged(data); });

    if (!mWifiAdapter->waitForConnection(timeoutMs)) {
        ESP_LOGE(Tag, "Failed to connect to WiFi");
        return false;
    }

    ESP_LOGI(Tag, "Creating SignalTask");
    mSignalTaskHandle = mTaskRunner.start(
        common::TaskParams{
            .name = "SignalTask", .priority = SignalTaskPriority, .core = SignalTaskCore},
        SignalTaskStackWords, &WifiService::signalStepFn, this);
    if (!mSignalTaskHandle.isValid()) {
        ESP_LOGE(Tag, "Failed to create SignalTask");
        return false;
    }

    ESP_LOGI(Tag, "WiFi connected!");
    return true;
}

bool WifiService::isConnected() const {
    return mWifiAdapter ? mWifiAdapter->isConnected() : false;
}

std::string WifiService::getStatus() const {
    return mWifiAdapter ? mWifiAdapter->getStatus() : "Not initialized";
}

void WifiService::disconnect() {
    (void)mTaskRunner.stop(mSignalTaskHandle, TimeoutToExitTasksMs);

    if (mWifiAdapter) {
        mWifiAdapter->deinit();
    }

    mLastBars = 0U;
}

common::StepResult WifiService::signalStepFn(void* arg, common::IStopToken& token) {
    auto* self = static_cast<WifiService*>(arg);
    if (!self) {
        return {.action = common::StepAction::Error};
    }

    return self->signalStep(token);
}

common::StepResult WifiService::signalStep(common::IStopToken& token) {
    if (token.stopRequested()) {
        return {.action = common::StepAction::Done};
    }

    const auto rssiOpt = mWifiAdapter->tryGetRssiDbm();
    const uint8_t bars = rssiToBars(rssiOpt.value_or(-100));
    if (bars != mLastBars) {
        mLastBars = bars;
        ESP_LOGI(Tag, "Signal: bars=%d", bars);
        mCoreEventQueue->post(common::WifiStateChangedEvent{.isConnected = true, .bars = bars});
    }

    return {.action = common::StepAction::Sleep, .sleepMs = 10000U};
}

void WifiService::onWifiStateChanged(const common::WifiData& data) {
    ESP_LOGI(Tag, "WiFi state changed: rssi=%d, isConnected=%d", data.rssi, data.isConnected);

    if (mCoreEventQueue) {
        mCoreEventQueue->post(common::WifiStateChangedEvent{.isConnected = data.isConnected,
                                                            .bars = rssiToBars(data.rssi)});
    }
}

}  // namespace services
