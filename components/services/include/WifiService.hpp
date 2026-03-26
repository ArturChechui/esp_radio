#pragma once

#include <string>

#include "IWifiService.hpp"
#include "Types.hpp"

namespace adapters {
class IPersistentStorage;
class IWifiClient;
class IProvisioningPortal;
}  // namespace adapters

namespace common {
class ITaskRunner;
class IEventQueue;
}  // namespace common

namespace services {
class WifiService final : public IWifiService {
   public:
    explicit WifiService(adapters::IWifiClient& wifiClient,
                         adapters::IProvisioningPortal& provisioningPortal,
                         common::ITaskRunner& taskRunner, common::IEventQueue& coreEventQueue,
                         adapters::IPersistentStorage& persistentStorage);
    ~WifiService() override = default;

    bool init() override;
    bool connect(const uint32_t timeoutMs) override;
    bool isConnected() const override;
    std::string getStatus() const override;
    void disconnect() override;
    bool startProvisioningPortal() override;

   private:
    static common::StepResult signalStepFn(void* arg, common::IStopToken& token);
    common::StepResult signalStep(common::IStopToken& token);
    void onWifiStateChanged(const common::WifiState& data);
    void onProvisioningCredentials(const common::WifiCredentials& data);

    adapters::IWifiClient& mWifiAdapter;
    adapters::IProvisioningPortal& mProvisioningPortal;
    common::ITaskRunner& mTaskRunner;
    common::IEventQueue& mCoreEventQueue;
    adapters::IPersistentStorage& mPersistentStorage;

    uint8_t mLastBars;
    common::TaskHandle mSignalTaskHandle;
    common::WifiCredentials mWifiCreds;
    bool mProvisioningRunning;
};

}  // namespace services
