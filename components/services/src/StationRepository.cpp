#include "StationRepository.hpp"

#include <algorithm>

// IDF
#include <esp_log.h>

namespace services {
static const char* TAG = "StationRepository";

StationRepository::StationRepository() : mStations(), mInitialized(false), mCurrentStationIdx(0U) {
    ESP_LOGI(TAG, "StationRepository created");
}

bool StationRepository::init() {
    if (mInitialized) {
        ESP_LOGW(TAG, "Already initialized, ignoring");
        return true;
    }

    // TODO: Load from LittleFS stations.json
    // For now, hardcoded stations for FR-01
    mStations = {{"hitfm_hd_1", "HitFM_HD", "https://online.hitfm.ua/HitFM_HD"},
                 {"hitfm_2", "HitFM", "https://online.hitfm.ua/HitFM"},
                 {"radio1_3", "Radio1", "http://play.global.audio/radio164"},
                 {"caroline_4", "Caroline", "https://stream.radiocaroline.net/;"},
                 {"luxfmhd_5", "LuxFM_HD", "http://icecast.luxnet.ua/luxlviv_hd"},
                 {"nasheradio_6", "NasheRadio", "http://online.nasheradio.ua/NasheRadio"}};

    // mCurrentStationIdx() -- read from persistance?

    ESP_LOGI(TAG, "Loaded %d stations", static_cast<int>(mStations.size()));

    mInitialized = true;
    return true;
}

const std::vector<common::StationData>& StationRepository::getStations() const {
    if (!mInitialized) {
        ESP_LOGW(TAG, "Not initialized yet");
    }

    return mStations;
}

const common::StationData& StationRepository::nextStation() {
    static const common::StationData kEmpty{"", "", ""};

    if (!mInitialized) {
        ESP_LOGW(TAG, "Not initialized yet");
        return kEmpty;
    }
    if (mStations.empty()) {
        ESP_LOGW(TAG, "No stations available");
        return kEmpty;
    }

    mCurrentStationIdx = (mCurrentStationIdx + 1U) % static_cast<uint32_t>(mStations.size());
    return mStations[mCurrentStationIdx];
}

const common::StationData& StationRepository::prevStation() {
    static const common::StationData kEmpty{"", "", ""};

    if (!mInitialized) {
        ESP_LOGW(TAG, "Not initialized yet");
        return kEmpty;
    }
    if (mStations.empty()) {
        ESP_LOGW(TAG, "No stations available");
        return kEmpty;
    }

    const uint32_t n = static_cast<uint32_t>(mStations.size());
    mCurrentStationIdx = (mCurrentStationIdx + n - 1U) % n;  // wrap backwards
    return mStations[mCurrentStationIdx];
}

const common::StationData& StationRepository::currentStation() const {
    static const common::StationData kEmpty{"", "", ""};

    if (!mInitialized) {
        ESP_LOGW(TAG, "Not initialized yet");
        return kEmpty;
    }
    if (mStations.empty()) {
        ESP_LOGW(TAG, "No stations available");
        return kEmpty;
    }

    const size_t idx = std::min<size_t>(mCurrentStationIdx, mStations.size() - 1U);
    return mStations[idx];
}

}  // namespace services
