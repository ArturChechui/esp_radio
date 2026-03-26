#include "StationRepository.hpp"

#include <algorithm>
#include <optional>
#include <utility>

// IDF
#include <esp_log.h>

#include "IFileSystem.hpp"
#include "IJsonParser.hpp"
#include "IPersistentStorage.hpp"
#include "LockGuard.hpp"

namespace services {
namespace {
constexpr const char* Tag = "StationRepository";
constexpr const char* CurrStationIdxStorageKey = "station_idx";
constexpr const char* StationJsonPath = "stations.json";
}  // namespace

StationRepository::StationRepository(adapters::IPersistentStorage& persistentStorage,
                                     adapters::IFileSystem& fileSystem, common::IJsonParser& parser)
    : mPersistentStorage(persistentStorage),
      mFileSystem(fileSystem),
      mParser(parser),
      mStations(),
      mInitialized(false),
      mCurrentStationIdx(0U),
      mMutex() {
    ESP_LOGI(Tag, "StationRepository created");
}

bool StationRepository::init() {
    common::LockGuard guard(mMutex);
    if (mInitialized) {
        return true;
    }

    if (refreshStationsFromFile()) {
        // if successfully read the stations, get the index
        if (!mPersistentStorage.getU32(CurrStationIdxStorageKey, mCurrentStationIdx) ||
            mCurrentStationIdx >= mStations.size()) {
            mCurrentStationIdx = 0U;
            (void)mPersistentStorage.setU32(CurrStationIdxStorageKey, mCurrentStationIdx);
        }
    }

    mInitialized = true;
    return true;
}

bool StationRepository::load() {
    common::LockGuard guard(mMutex);
    if (!mInitialized) {
        return false;
    }

    std::string playingId = mStations.empty() ? "" : mStations[mCurrentStationIdx].id;

    if (!refreshStationsFromFile()) {
        return false;
    }

    auto it = std::find_if(mStations.begin(), mStations.end(), [&playingId](const auto& s) {
        return !playingId.empty() && (s.id == playingId);
    });
    if (it != mStations.end()) {
        mCurrentStationIdx = static_cast<uint32_t>(std::distance(mStations.begin(), it));
    } else {
        ESP_LOGI(Tag, "Active station lost in sync, resetting to 0");
        mCurrentStationIdx = 0U;
    }
    (void)mPersistentStorage.setU32(CurrStationIdxStorageKey, mCurrentStationIdx);

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

    if (!mPersistentStorage.setU32(CurrStationIdxStorageKey, mCurrentStationIdx)) {
        ESP_LOGW(Tag, "Failed to save current station index to persistent storage");
    }

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

    if (!mPersistentStorage.setU32(CurrStationIdxStorageKey, mCurrentStationIdx)) {
        ESP_LOGW(Tag, "Failed to save current station index to persistent storage");
    }

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

bool StationRepository::refreshStationsFromFile() {
    std::string data;
    if (!mFileSystem.readFile(StationJsonPath, data) || data.empty()) {
        ESP_LOGE(Tag, "Failed to read %s", StationJsonPath);
        return false;
    }

    std::vector<common::StationData> nextStations;
    if (!mParser.parseStations(data, nextStations) || nextStations.empty()) {
        ESP_LOGE(Tag, "Failed to parse stations");
        return false;
    }

    mStations = std::move(nextStations);
    return true;
}

}  // namespace services
