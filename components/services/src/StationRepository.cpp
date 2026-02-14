#include "StationRepository.hpp"

#include <algorithm>

// IDF
#include <esp_log.h>

#include "LockGuard.hpp"

namespace services {
static const char* Tag = "StationRepository";

StationRepository::StationRepository()
    : mStations(), mInitialized(false), mCurrentStationIdx(0U), mMutex() {
    ESP_LOGI(Tag, "StationRepository created");
}

bool StationRepository::init() {
    common::LockGuard guard(mMutex);

    if (mInitialized) {
        ESP_LOGW(Tag, "Already initialized, ignoring");
        return true;
    }

    // TODO: Load from LittleFS stations.json
    // For now, hardcoded stations for FR-01
    mStations = {{"hitfm_1", "Hit FM", "https://online.hitfm.ua/HitFM"},
                 {"hitfmhd_2", "HitHD", "https://online.hitfm.ua/HitFM_HD"},
                 {"kissfm_3", "KissFM", "http://online.kissfm.ua/KissFM"},
                 {"kissfmhd_4", "KissHD", "https://online.kissfm.ua/KissFM_HD"},
                 {"luxfmhd_5", "LuxHD", "http://icecast.luxnet.ua/luxlviv_hd"},
                 {"relax_6", "Relax", "https://online.radiorelax.ua/RadioRelax_Ukr"},
                 {"pyatnica_7", "Friday", "https://cast.mediaonline.net.ua/radiopyatnica"},
                 {"nasheradio_8", "Nashe", "http://online.nasheradio.ua/NasheRadio"}};

    // mCurrentStationIdx() -- read from persistance?

    ESP_LOGI(Tag, "Loaded %d stations", static_cast<int>(mStations.size()));

    mInitialized = true;
    return true;
}

const std::vector<common::StationData>& StationRepository::getStations() const {
    common::LockGuard guard(mMutex);

    if (!mInitialized) {
        ESP_LOGW(Tag, "Not initialized yet");
    }

    return mStations;
}

const common::StationData& StationRepository::nextStation() {
    common::LockGuard guard(mMutex);

    static const common::StationData kEmpty{"", "", ""};

    if (!mInitialized) {
        ESP_LOGW(Tag, "Not initialized yet");
        return kEmpty;
    }
    if (mStations.empty()) {
        ESP_LOGW(Tag, "No stations available");
        return kEmpty;
    }

    mCurrentStationIdx = (mCurrentStationIdx + 1U) % static_cast<uint32_t>(mStations.size());
    ESP_LOGI(Tag, "nextStation: name=%s, idx=%u", mStations[mCurrentStationIdx].name.c_str(),
             static_cast<unsigned>(mCurrentStationIdx));

    return mStations[mCurrentStationIdx];
}

const common::StationData& StationRepository::prevStation() {
    common::LockGuard guard(mMutex);

    static const common::StationData kEmpty{"", "", ""};

    if (!mInitialized) {
        ESP_LOGW(Tag, "Not initialized yet");
        return kEmpty;
    }
    if (mStations.empty()) {
        ESP_LOGW(Tag, "No stations available");
        return kEmpty;
    }

    const uint32_t n = static_cast<uint32_t>(mStations.size());
    mCurrentStationIdx = (mCurrentStationIdx + n - 1U) % n;  // wrap backwards
    ESP_LOGI(Tag, "prevStation: name=%s, idx=%u", mStations[mCurrentStationIdx].name.c_str(),
             static_cast<unsigned>(mCurrentStationIdx));

    return mStations[mCurrentStationIdx];
}

const common::StationData& StationRepository::currentStation() const {
    common::LockGuard guard(mMutex);

    static const common::StationData kEmpty{"", "", ""};

    if (!mInitialized) {
        ESP_LOGW(Tag, "Not initialized yet");
        return kEmpty;
    }
    if (mStations.empty()) {
        ESP_LOGW(Tag, "No stations available");
        return kEmpty;
    }

    const uint32_t idx = std::min<uint32_t>(mCurrentStationIdx, mStations.size() - 1U);
    ESP_LOGI(Tag, "currentStation: name=%s, idx=%u", mStations[idx].name.c_str(),
             static_cast<unsigned>(idx));

    return mStations[idx];
}

}  // namespace services
