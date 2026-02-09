#include "WifiService.hpp"

#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ITaskRunner.hpp"

namespace services {
namespace {
static int rssiToBars(int rssiDbm) {
    if (rssiDbm >= -55)
        return 4;
    if (rssiDbm >= -65)
        return 3;
    if (rssiDbm >= -75)
        return 2;
    if (rssiDbm >= -85)
        return 1;
    return 0;
}

static const char* Tag = "WifiService";

constexpr int SignalTaskPriority = 5;
constexpr int SignalTaskCore = 0;
constexpr uint32_t SignalTaskStackWords = 2304U;  // 16 KB
constexpr uint32_t TimeoutToExitTasks = 7000U;    // 7s
}  // namespace

WifiService::WifiService(std::shared_ptr<adapters::IWifiClient> adapter,
                         common::ITaskRunner& taskRunner)
    : mWifiAdapter(adapter),
      mCoreEventQueue(nullptr),
      mLastBars(-1),
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

    // Register callback for WiFi state changes
    mWifiAdapter->setStateCallback(
        [this](const adapters::WifiStateChangedEvent& event) { this->onWifiStateChanged(event); });

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
    (void)mTaskRunner.stop(mSignalTaskHandle, TimeoutToExitTasks);

    if (mWifiAdapter) {
        mWifiAdapter->deinit();
    }

    mLastBars = -1;
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
    const int bars = rssiOpt ? rssiToBars(*rssiOpt) : 0;
    const int rssi = rssiOpt.value_or(-100);

    if (bars != mLastBars) {
        mLastBars = bars;
        ESP_LOGI(Tag, "Signal: rssi=%d dBm, bars=%d", rssi, bars);

        // later:
        // mCoreEventQueue.post(WifiSignalChangedEvent{.rssiDbm=rssi, .bars=bars});
    }

    return {.action = common::StepAction::Sleep, .sleepMs = 10000U};  // 10s
}

void WifiService::onWifiStateChanged(const adapters::WifiStateChangedEvent& event) {
    ESP_LOGI(Tag, "WiFi state changed: isConnected=%d, ssid=%s", event.isConnected,
             event.ssid.c_str());

    // Notify any service that's listening
    if (mCoreEventQueue) {
        common::WifiStateChangedEvent coreEvent{event.isConnected};
        mCoreEventQueue->post(coreEvent);
    }
}

}  // namespace services
