#include "AppContext.hpp"

#include <esp_log.h>
#include <esp_netif.h>
#include <nvs_flash.h>

#include "RingBuffer.hpp"
#include "Signal.hpp"

namespace core {
namespace {
constexpr const char* Tag = "AppContext";
}  // namespace

AppContext::AppContext() {}

bool AppContext::init() {
    ESP_LOGI(Tag, "=== Initializing AppContext ===");

    if (!initNvsFlash()) {
        ESP_LOGE(Tag, "Failed to init nvs flash");
        return false;
    }
    if (!initCommon()) {
        ESP_LOGE(Tag, "Failed to init common");
        return false;
    }
    if (!initAdapters()) {
        ESP_LOGE(Tag, "Failed to init adapters");
        return false;
    }
    if (!initServices()) {
        ESP_LOGE(Tag, "Failed to init services");
        return false;
    }
    if (!initCore()) {
        ESP_LOGE(Tag, "Failed to init core");
        return false;
    }
    if (!mUiEventTask->run(*mUiService) || !mCoreEventTask->run(*mAppController)) {
        ESP_LOGE(Tag, "Failed to run EventTasks");
        return false;
    }

    mCoreEventTask->post(common::SystemInitedEvent{});

    return true;
}

bool AppContext::initNvsFlash() {
    ESP_LOGI(Tag, "Initializing NVS");

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

    return true;
}

bool AppContext::initCommon() {
    ESP_LOGI(Tag, "Initializing Common");

    mTaskRunner = std::make_unique<common::TaskRunner>();
    mStats = std::make_unique<common::AudioBufferStats>(10000U);
    mClock = std::make_unique<common::Clock>();
    mQueue = std::make_unique<common::Queue<uint32_t>>("InputQueue");
    mUiEventTask = std::make_unique<common::EventTask>("UiEventTask", *mTaskRunner);
    mCoreEventTask = std::make_unique<common::EventTask>("CoreEventTask", *mTaskRunner);
    mJsonParser = std::make_unique<common::JsonParser>();

    if (!mUiEventTask->init() || !mCoreEventTask->init()) {
        ESP_LOGE(Tag, "Failed to init EventTasks");
        return false;
    }

    return true;
}

bool AppContext::initAdapters() {
    ESP_LOGI(Tag, "Initializing Adapters");

    mPersistentStorage = std::make_unique<adapters::PersistentStorage>();
    mWifiAdapter = std::make_unique<adapters::WifiClient>();
    mProvisioningPortal = std::make_unique<adapters::ProvisioningPortal>(*mWifiAdapter);
    mI2cBus = std::make_unique<adapters::I2cBus>();
    mI2sBus = std::make_unique<adapters::I2sBus>();
    mStreamHttpClient = std::make_unique<adapters::HttpClient>();
    mJsonHttpClient = std::make_unique<adapters::HttpClient>();
    mMp3Decoder = std::make_unique<adapters::Mp3Decoder>();
    mDisplay = std::make_unique<adapters::Display>(*mI2cBus);
    mGpioInput = std::make_unique<adapters::GpioInput>(*mQueue);
    mAdcReader = std::make_unique<adapters::AdcReader>();
    mFileSystem = std::make_unique<adapters::FileSystem>();

    if (!mFileSystem->init() || !mPersistentStorage->init() || !mWifiAdapter->init() ||
        !mI2sBus->init() || !mI2cBus->init() || !mDisplay->init() || !mGpioInput->init() ||
        !mAdcReader->init()) {
        ESP_LOGE(Tag, "Failed to init adapters");
        return false;
    }

    return true;
}

bool AppContext::initServices() {
    ESP_LOGI(Tag, "Initializing Services");

    mWifiService = std::make_unique<services::WifiService>(
        *mWifiAdapter, *mProvisioningPortal, *mTaskRunner, *mCoreEventTask, *mPersistentStorage);
    mSensorService = std::make_unique<services::SensorService>(
        *mI2cBus, *mAdcReader, *mCoreEventTask, *mTaskRunner, *mClock);
    mInputService = std::make_unique<services::InputService>(
        *mGpioInput, *mCoreEventTask, *mQueue, *mTaskRunner, *mClock, *mPersistentStorage);
    mPlayerService = std::make_unique<services::PlayerService>(
        *mI2sBus, *mStreamHttpClient, *mMp3Decoder, *mTaskRunner,
        std::make_unique<common::RingBuffer>(services::PlayerService::RingBufferSize), *mStats,
        *mCoreEventTask,
        std::make_unique<common::Signal>());  // TODO: move signal inside, no need
                                              // to have it outside, make similar to Mutex
    mStationRepository = std::make_unique<services::StationRepository>(*mPersistentStorage,
                                                                       *mFileSystem, *mJsonParser);
    mUiService = std::make_unique<services::UiService>(*mDisplay, *mStationRepository);

    if (!mWifiService->init() || !mSensorService->init() || !mInputService->init() ||
        !mStationRepository->init() || !mUiService->init()) {
        ESP_LOGE(Tag, "Failed to init services");
        return false;
    }

    return true;
}

bool AppContext::initCore() {
    ESP_LOGI(Tag, "Initializing Core");

    mAppController = std::make_unique<AppController>(
        *mWifiService, *mPlayerService, *mStationRepository, *mSensorService, *mInputService,
        *mJsonHttpClient, *mFileSystem, *mJsonParser, *mUiEventTask);

    return true;
}

}  // namespace core
