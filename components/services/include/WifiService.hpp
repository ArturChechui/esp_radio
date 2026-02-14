#pragma once

#include <functional>
#include <memory>
#include <string>

#include "IEventQueue.hpp"
#include "IStopToken.hpp"
#include "IWifiClient.hpp"
#include "Types.hpp"

namespace common {
class ITaskRunner;
}  // namespace common

namespace services {
class WifiService {
   public:
    explicit WifiService(std::shared_ptr<adapters::IWifiClient> adapter,
                         common::ITaskRunner& taskRunner);

    bool connect(const std::string& ssid, const std::string& password,
                 common::IEventQueue& coreEventQueue, uint32_t timeoutMs = 30000);
    bool isConnected() const;
    std::string getStatus() const;
    void disconnect();

   private:
    static common::StepResult signalStepFn(void* arg, common::IStopToken& token);
    common::StepResult signalStep(common::IStopToken& token);

    void onWifiStateChanged(const common::WifiData& data);

    std::shared_ptr<adapters::IWifiClient> mWifiAdapter;
    common::IEventQueue* mCoreEventQueue;
    uint8_t mLastBars;
    common::TaskHandle mSignalTaskHandle;
    common::ITaskRunner& mTaskRunner;
};

}  // namespace services
