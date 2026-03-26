#include "WifiService.hpp"

#include <esp_log.h>

#include "IEventQueue.hpp"
#include "IPersistentStorage.hpp"
#include "IProvisioningPortal.hpp"
#include "IStopToken.hpp"
#include "ITaskRunner.hpp"
#include "IWifiClient.hpp"

namespace services {
namespace {
constexpr const char* Tag = "WifiService";
constexpr const char* TaskName = "SignalTask";
constexpr int TaskPriority = 5;
constexpr int TaskCore = 0;
constexpr uint32_t TaskStackWords = 3456U;
constexpr uint32_t TimeoutToExitTasksMs = 7000U;

constexpr const char* ProvisioningApSsid = "SetupESPW";
constexpr const char* ProvisioningApPassword = "";
constexpr uint8_t ProvisioningApChannel = 1U;
constexpr uint8_t ProvisioningApMaxConnections = 1U;

constexpr const char* SsidStorageKey = "wifi_ssid";
constexpr const char* PasswordStorageKey = "wifi_password";

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

WifiService::WifiService(adapters::IWifiClient& wifiClient,
                         adapters::IProvisioningPortal& provisioningPortal,
                         common::ITaskRunner& taskRunner, common::IEventQueue& coreEventQueue,
                         adapters::IPersistentStorage& persistentStorage)
    : mWifiAdapter(wifiClient),
      mProvisioningPortal(provisioningPortal),
      mTaskRunner(taskRunner),
      mCoreEventQueue(coreEventQueue),
      mPersistentStorage(persistentStorage),
      mLastBars(0U),
      mSignalTaskHandle(),
      mWifiCreds(),
      mProvisioningRunning(false) {}

bool WifiService::init() {
    std::string ssid;
    std::string password;

    mPersistentStorage.getString(SsidStorageKey, ssid);
    if (ssid.empty()) {
        ESP_LOGW(Tag, "ssid from persistent storage is empty");
    }

    mPersistentStorage.getString(PasswordStorageKey, password);
    if (password.empty()) {
        ESP_LOGW(Tag, "password from persistent storage is empty");
    }

    mWifiCreds.ssid = std::move(ssid);
    mWifiCreds.password = std::move(password);

    return true;
}

bool WifiService::connect(const uint32_t timeoutMs) {
    if (mProvisioningRunning && mProvisioningPortal.isRunning()) {
        mProvisioningPortal.stop();
        mProvisioningRunning = false;
    }

    ESP_LOGI(Tag, "Connecting to WiFi \"%s\"", mWifiCreds.ssid.c_str());

    mWifiAdapter.setStateCallback(
        [this](const common::WifiState& data) { this->onWifiStateChanged(data); });

    if (!mWifiAdapter.connect(mWifiCreds)) {
        ESP_LOGE(Tag, "Failed to start WiFi connection");
        return false;
    }

    if (!mWifiAdapter.waitForConnection(timeoutMs)) {
        ESP_LOGE(Tag, "Failed to connect to WiFi");
        disconnect();
        return false;
    }

    ESP_LOGI(Tag, "Creating SignalTask");
    mSignalTaskHandle = mTaskRunner.start(
        common::TaskParams{.name = TaskName, .priority = TaskPriority, .core = TaskCore},
        TaskStackWords, &WifiService::signalStepFn, this);
    if (!mSignalTaskHandle.isValid()) {
        ESP_LOGW(Tag, "Failed to create SignalTask");
    }

    ESP_LOGI(Tag, "WiFi connected!");
    mProvisioningRunning = false;

    return true;
}

bool WifiService::isConnected() const {
    return mWifiAdapter.isConnected();
}

std::string WifiService::getStatus() const {
    return mWifiAdapter.getStatus();
}

void WifiService::disconnect() {
    if (mSignalTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mSignalTaskHandle, TimeoutToExitTasksMs);
        mSignalTaskHandle.reset();
    }

    (void)mWifiAdapter.disconnect();

    if (mProvisioningPortal.isRunning()) {
        mProvisioningPortal.stop();
    }
    mProvisioningRunning = false;

    mLastBars = 0U;
}

bool WifiService::startProvisioningPortal() {
    common::ProvisioningPortalConfig cfg{
        .apSsid = ProvisioningApSsid,
        .apPassword = ProvisioningApPassword,
        .channel = ProvisioningApChannel,
        .maxConnections = ProvisioningApMaxConnections,
    };

    const bool started = mProvisioningPortal.start(
        cfg, [this](const common::WifiCredentials& creds) { onProvisioningCredentials(creds); });
    if (!started) {
        ESP_LOGE(Tag, "Failed to start provisioning portal");
        return false;
    }

    mProvisioningRunning = true;

    return true;
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

    const auto rssiOpt = mWifiAdapter.tryGetRssiDbm();
    const uint8_t bars = rssiToBars(rssiOpt.value_or(-100));
    if (bars != mLastBars) {
        mLastBars = bars;
        ESP_LOGI(Tag, "Signal: bars=%d", bars);
        mCoreEventQueue.post(common::WifiStateChangedEvent{.isConnected = true, .bars = bars});
    }

    return {.action = common::StepAction::Sleep, .sleepMs = 10000U};
}

void WifiService::onWifiStateChanged(const common::WifiState& data) {
    ESP_LOGI(Tag, "WiFi state changed: rssi=%d, isConnected=%d", data.rssi, data.isConnected);

    mCoreEventQueue.post(common::WifiStateChangedEvent{.isConnected = data.isConnected,
                                                       .bars = rssiToBars(data.rssi)});
}

void WifiService::onProvisioningCredentials(const common::WifiCredentials& data) {
    ESP_LOGI(Tag, "Provisioning credentials received for SSID '%s'", data.ssid.c_str());

    mPersistentStorage.setString(SsidStorageKey, data.ssid);
    mPersistentStorage.setString(PasswordStorageKey, data.password);
    mWifiCreds = data;

    mCoreEventQueue.post(common::WifiCredsReceivedEvent{});
}

}  // namespace services
