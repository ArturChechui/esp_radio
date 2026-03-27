#pragma once

#include <esp_adc/adc_oneshot.h>

#include <cstddef>
#include <cstdint>
#include <map>

#include "IAdcReader.hpp"
#include "Mutex.hpp"

namespace adapters {
class AdcReader final : public IAdcReader {
   public:
    AdcReader();
    ~AdcReader() override;

    bool init() override;
    bool setupChannel(const uint32_t gpioNum) override;
    bool readRawBurst(const uint32_t gpioNum, int* buffer, const size_t count) override;

   private:
    struct ChannelInfo {
        adc_channel_t channel;
        bool valid = false;
    };

    std::map<uint32_t, ChannelInfo> mLookup;
    adc_oneshot_unit_handle_t mUnitHandle;
    bool mIsInitialized;
    common::Mutex mMutex;
};

}  // namespace adapters
