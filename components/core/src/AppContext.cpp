#include "AppContext.hpp"

#include <esp_log.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <sdkconfig.h>

#include "RingBuffer.hpp"
#include "Signal.hpp"

namespace core {
namespace {
constexpr const char* Tag = "AppContext";
}  // namespace

AppContext::AppContext() {}

bool AppContext::init() {
    ESP_LOGI(Tag, "=== Initializing AppContext ===");

    // STEP 1: Initialize NVS (MUST be first, before any service uses it)
    ESP_LOGI(Tag, "Step 1: Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(Tag, "NVS needs erasing, wiping...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(Tag, "NVS init failed: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(Tag, "NVS initialized");

    // STEP 2: Common components
    mTaskRunner = std::make_unique<common::TaskRunner>();
    mStats = std::make_unique<common::AudioBufferStats>(10000U);
    mClock = std::make_unique<common::Clock>();
    mQueue = std::make_unique<common::Queue<uint32_t>>("InputQueue");

    // STEP 3: Initialize WiFi
    ESP_LOGI(Tag, "Step 2: Initializing WiFi...");
    mWifiAdapter = std::make_shared<adapters::WifiClient>();
    mWifiService = std::make_shared<services::WifiService>(mWifiAdapter, *mTaskRunner);

    // STEP 4: Create adapters
    mI2cBus = std::make_unique<adapters::I2cBus>();
    mI2sBus = std::make_unique<adapters::I2sBus>();
    mHttpClient = std::make_unique<adapters::HttpClient>();
    mMp3Decoder = std::make_unique<adapters::Mp3Decoder>();
    mDisplay = std::make_unique<adapters::Display>(*mI2cBus);
    mGpioInput = std::make_unique<adapters::GpioInput>(*mQueue);
    if (!mI2sBus->init() || !mI2cBus->init() || !mDisplay->init() || !mGpioInput->init()) {
        ESP_LOGE(Tag, "Failed to init adapters");
        return false;
    }

    // STEP 5: Create tasks (no init yet)
    mUiEventTask = std::make_unique<common::EventTask>("UiEventTask", *mTaskRunner);
    mCoreEventTask = std::make_unique<common::EventTask>("CoreEventTask", *mTaskRunner);

    // STEP 6: Create services
    mSensorService =
        std::make_unique<services::SensorService>(*mI2cBus, *mCoreEventTask, *mTaskRunner);
    mInputService = std::make_unique<services::InputService>(*mGpioInput, *mCoreEventTask, *mQueue,
                                                             *mTaskRunner, *mClock);
    mPlayerService = std::make_unique<services::PlayerService>(
        *mI2sBus, *mHttpClient, *mMp3Decoder, *mTaskRunner,
        std::make_unique<common::RingBuffer>(services::PlayerService::RingBufferSize), *mStats,
        *mCoreEventTask, std::make_unique<common::Signal>());
    mStationRepository = std::make_unique<services::StationRepository>();
    mUiService = std::make_unique<services::UiService>(*mDisplay, *mStationRepository);
    ESP_LOGI(Tag, "UiService is disabled for testing.");
    if (!mPlayerService->init() || !mStationRepository->init() || !mUiService->init() ||
        !mInputService->init() || !mSensorService->init()) {
        ESP_LOGE(Tag, "Failed to init services");
        return false;
    }

    // STEP 7: Create controller
    mAppController =
        std::make_unique<AppController>(*mPlayerService, *mStationRepository, *mUiEventTask);
    if (!mAppController->init()) {
        ESP_LOGE(Tag, "Failed to init controller");
        return false;
    }

    // STEP 8: Init tasks
    if (!mUiEventTask->init(*mUiService) || !mCoreEventTask->init(*mAppController)) {
        ESP_LOGE(Tag, "Failed to init tasks");
        return false;
    }

    // TODO: move to AppController to do it on SystemReady event
    // STEP 7: Connect to WiFi (temporary here, later will be in a cmd)
    if (!mWifiService->connect(CONFIG_ESP_RADIO_WIFI_SSID, CONFIG_ESP_RADIO_WIFI_PASS,
                               *mCoreEventTask, 30000U)) {
        ESP_LOGE(Tag, "WiFi connection failed");
        return false;
    }
    ESP_LOGI(Tag, "%s", mWifiService->getStatus().c_str());

    // Delay to allow services to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));

    mCoreEventTask->post(common::SystemReadyEvent{true});

    ESP_LOGI(Tag, "AppContext initialized");
    return true;
}

}  // namespace core
