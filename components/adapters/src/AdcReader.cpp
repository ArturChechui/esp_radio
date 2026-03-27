#include "AdcReader.hpp"

#include <esp_log.h>

#include <algorithm>

#include "LockGuard.hpp"

namespace adapters {
namespace {
constexpr const char* Tag = "AdcReader";
}  // namespace

AdcReader::AdcReader() : mLookup(), mUnitHandle(nullptr), mIsInitialized(false), mMutex() {
    ESP_LOGI(Tag, "AdcReader created");
}

AdcReader::~AdcReader() {
    if (mUnitHandle) {
        (void)adc_oneshot_del_unit(mUnitHandle);
        mUnitHandle = nullptr;
    }
    mIsInitialized = false;
}

bool AdcReader::init() {
    common::LockGuard guard(mMutex);

    if (mIsInitialized && mUnitHandle) {
        return true;
    }

    adc_oneshot_unit_init_cfg_t initCfg = {};
    initCfg.unit_id = ADC_UNIT_1;
    initCfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    const esp_err_t ret = adc_oneshot_new_unit(&initCfg, &mUnitHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(Tag, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        mUnitHandle = nullptr;
        mIsInitialized = false;
        return false;
    }

    mIsInitialized = true;
    ESP_LOGI(Tag, "AdcReader initialized");

    return true;
}

bool AdcReader::setupChannel(const uint32_t gpioNum) {
    if (!mIsInitialized && !init()) {
        return false;
    }

    common::LockGuard guard(mMutex);

    if (!mIsInitialized) {
        return false;
    }

    adc_unit_t unit;
    adc_channel_t channel;
    esp_err_t ret = adc_oneshot_io_to_channel(static_cast<int>(gpioNum), &unit, &channel);
    if (ret != ESP_OK) {
        ESP_LOGE(Tag, "adc_oneshot_io_to_channel failed for GPIO %u: %s",
                 static_cast<unsigned>(gpioNum), esp_err_to_name(ret));
        return false;
    }
    if (unit != ADC_UNIT_1) {
        ESP_LOGE(Tag, "GPIO %u mapped to unsupported ADC unit %d", static_cast<unsigned>(gpioNum),
                 static_cast<int>(unit));
        return false;
    }

    adc_oneshot_chan_cfg_t chanCfg = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    ret = adc_oneshot_config_channel(mUnitHandle, channel, &chanCfg);
    if (ret != ESP_OK) {
        ESP_LOGE(Tag, "adc_oneshot_config_channel failed for GPIO %u: %s",
                 static_cast<unsigned>(gpioNum), esp_err_to_name(ret));
        return false;
    }

    mLookup[gpioNum] = {.channel = channel, .valid = true};
    return true;
}

bool AdcReader::readRawBurst(const uint32_t gpioNum, int* buffer, const size_t count) {
    common::LockGuard guard(mMutex);

    auto it = mLookup.find(gpioNum);
    if (it == mLookup.end() || !it->second.valid) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        const esp_err_t ret = adc_oneshot_read(mUnitHandle, it->second.channel, &buffer[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(Tag, "adc_oneshot_read failed for GPIO %u: %s", static_cast<unsigned>(gpioNum),
                     esp_err_to_name(ret));
            return false;
        }
    }

    return true;
}

}  // namespace adapters
