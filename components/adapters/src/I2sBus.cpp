#include "I2sBus.hpp"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "BoardConfig.hpp"

namespace adapters {

namespace {
constexpr const char* Tag = "I2sBus";
constexpr int I2sDmaDescriptorCount = 12;
constexpr int I2sDmaFrameSize = 256;
}  // namespace

I2sBus::I2sBus() : mI2sTxHandle(nullptr), mCurrentSampleRate(common::I2S_SAMPLE_RATE) {
    ESP_LOGI(Tag, "I2sBus created");
}

I2sBus::~I2sBus() {
    deinit();
}

bool I2sBus::init() {
    if (mI2sTxHandle) {
        ESP_LOGW(Tag, "I2S already initialized");
        return true;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 12;     // 12
    chan_cfg.dma_frame_num = 1023;  // 256
    chan_cfg.auto_clear = true;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &mI2sTxHandle, nullptr));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(common::I2S_SAMPLE_RATE),
        .slot_cfg =
            I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg =
            {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = (gpio_num_t)common::I2S_BCLK_GPIO,
                .ws = (gpio_num_t)common::I2S_LRCK_GPIO,
                .dout = (gpio_num_t)common::I2S_DOUT_GPIO,
                .din = I2S_GPIO_UNUSED,
                .invert_flags =
                    {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv = false,
                    },
            },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(mI2sTxHandle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(mI2sTxHandle));

    ESP_LOGI(Tag, "I2S initialized (SR=%lu, BCLK=%u, LRCK=%u, DOUT=%u)", mCurrentSampleRate,
             common::I2S_BCLK_GPIO, common::I2S_LRCK_GPIO, common::I2S_DOUT_GPIO);

    return true;
}

void I2sBus::deinit() {
    if (!mI2sTxHandle) {
        return;
    }

    i2s_channel_disable(mI2sTxHandle);
    i2s_del_channel(mI2sTxHandle);
    mI2sTxHandle = nullptr;

    ESP_LOGI(Tag, "I2S deinitialized");
}

size_t I2sBus::write(const int16_t* data, const size_t& size, const uint32_t& timeoutMs) {
    if (!mI2sTxHandle || !data || size == 0UL) {
        ESP_LOGW(Tag, "I2S not initialized or invalid parameters");
        return 0;
    }

    size_t written = 0;
    const esp_err_t ret =
        i2s_channel_write(mI2sTxHandle, data, size, &written, pdMS_TO_TICKS(timeoutMs));
    if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
        ESP_LOGW(Tag, "i2s_channel_write failed: %s", esp_err_to_name(ret));
    }

    return written;
}

bool I2sBus::reconfigureClock(const uint32_t& sampleRate) {
    if (!mI2sTxHandle || sampleRate == 0U || sampleRate == mCurrentSampleRate) {
        ESP_LOGW(Tag, "Either I2S not initialized or sample rate unchanged");
        return false;
    }

    //  DISABLE before reconfiguring
    esp_err_t ret = i2s_channel_disable(mI2sTxHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(Tag, "i2s_channel_disable failed: %s", esp_err_to_name(ret));
        return false;
    }

    i2s_std_clk_config_t clkCfg = {};
    clkCfg.sample_rate_hz = sampleRate;
    clkCfg.clk_src = I2S_CLK_SRC_DEFAULT;
    clkCfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    clkCfg.ext_clk_freq_hz = 0;
    ret = i2s_channel_reconfig_std_clock(mI2sTxHandle, &clkCfg);
    if (ret != ESP_OK) {
        ESP_LOGW(Tag, "I2S clock reconfig failed (%lu -> %lu): %s", mCurrentSampleRate, sampleRate,
                 esp_err_to_name(ret));
        return false;
    }

    //  RE-ENABLE after reconfig
    ret = i2s_channel_enable(mI2sTxHandle);
    if (ret != ESP_OK) {
        ESP_LOGE(Tag, "i2s_channel_enable after reconfig failed: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(Tag, "I2S clock reconfigured: %lu -> %lu Hz", mCurrentSampleRate, sampleRate);
    mCurrentSampleRate = sampleRate;

    return true;
}

uint32_t I2sBus::getSampleRate() const {
    return mCurrentSampleRate;
}

}  // namespace adapters
