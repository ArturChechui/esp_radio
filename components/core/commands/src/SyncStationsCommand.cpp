#include "SyncStationsCommand.hpp"

#include <esp_log.h>

#include <string>
#include <vector>

#include "Events.hpp"
#include "IEventQueue.hpp"
#include "IFileSystem.hpp"
#include "IHttpClient.hpp"
#include "IJsonParser.hpp"
#include "IPlayerService.hpp"
#include "IStationRepository.hpp"
#include "IWifiService.hpp"
#include "Overloaded.hpp"

namespace core::commands {
namespace {
constexpr const char* Tag = "SyncStationsCmd";

// TODO: change to the main
constexpr const char* ManifestUrl =
    "https://raw.githubusercontent.com/ArturChechui/esp_radio/refs/heads/feature/FR-07/resources/"
    "json/manifest.json";
constexpr const char* StationsUrl =
    "https://raw.githubusercontent.com/ArturChechui/esp_radio/refs/heads/feature/FR-07/resources/"
    "json/stations.json";

constexpr const char* ManifestPath = "manifest.json";
constexpr const char* ManifestTmpPath = "manifest.new.json";
constexpr const char* ManifestBackupPath = "manifest.bak.json";

constexpr const char* StationsPath = "stations.json";
constexpr const char* StationsTmpPath = "stations.new.json";
constexpr const char* StationsBackupPath = "stations.bak.json";

constexpr uint32_t DownloadTimeoutMs = 15000U;

static bool isPlayingOrBuffering(common::PlaybackStatus s) {
    return (s == common::PlaybackStatus::Buffering) || (s == common::PlaybackStatus::Playing);
}
}  // namespace

SyncStationsCommand::SyncStationsCommand(services::IPlayerService& playerService,
                                         services::IWifiService& wifiService,
                                         adapters::IHttpClient& httpClient,
                                         adapters::IFileSystem& fileSystem,
                                         common::IJsonParser& jsonParser,
                                         services::IStationRepository& stationRepository,
                                         common::IEventQueue& uiEventQueue)
    : mPlayerService(playerService),
      mWifiService(wifiService),
      mHttpClient(httpClient),
      mFileSystem(fileSystem),
      mJsonParser(jsonParser),
      mStationRepository(stationRepository),
      mUiEventQueue(uiEventQueue),
      mStarted(false),
      mFinished(false),
      mRemoteManifestJson() {}

void SyncStationsCommand::handle(const common::AppEvent& e) {
    if (mFinished) {
        return;
    }

    if (mStarted) {
        std::visit(common::Overloaded{[this](const common::PlaybackStatusChangedEvent& p) {
                                          onPlaybackStatus(p.status);
                                      },
                                      [](const auto&) {}},
                   e);
        return;
    }

    mStarted = true;
    (void)mUiEventQueue.post(common::SwitchToSyncInProgressScreenEvent{});

    // TODO: Improve the repeating func
    if (stopPlaybackIfStarted()) {
        ESP_LOGI(Tag, "Playback is active, wait until stopped");
    } else {
        ESP_LOGI(Tag, "Starting station sync");
        if (runSync()) {
            ESP_LOGI(Tag, "Sync completed");
        } else {
            ESP_LOGE(Tag, "Sync failed");
        }

        finish();
    }
}

bool SyncStationsCommand::isFinished() {
    return mFinished;
}

bool SyncStationsCommand::stopPlaybackIfStarted() {
    bool needToWaitStop = false;

    const auto st = mPlayerService.getStatus();
    if (isPlayingOrBuffering(st)) {
        (void)mPlayerService.stop();
        needToWaitStop = true;
    }

    return needToWaitStop;
}

void SyncStationsCommand::onPlaybackStatus(common::PlaybackStatus s) {
    if (s == common::PlaybackStatus::Error) {
        ESP_LOGW(Tag, "Finished on Error");
    } else if (s == common::PlaybackStatus::Stopped) {
        ESP_LOGI(Tag, "Playback stopped. Starting station sync");
        if (runSync()) {
            ESP_LOGI(Tag, "Sync completed");
        } else {
            ESP_LOGE(Tag, "Sync failed");
        }
    }

    finish();
}

bool SyncStationsCommand::runSync() {
    if (!mWifiService.isConnected()) {
        ESP_LOGW(Tag, "Sync aborted: WiFi disconnected");
        return false;
    }

    mRemoteManifestJson.clear();
    if (!mHttpClient.download(ManifestUrl, mRemoteManifestJson, DownloadTimeoutMs)) {
        ESP_LOGW(Tag, "Failed to download remote manifest");
        return false;
    }

    bool manifestsAreDifferent = false;
    if (!manifestsDifferent(manifestsAreDifferent)) {
        return false;
    }

    if (!manifestsAreDifferent) {
        ESP_LOGI(Tag, "Manifest unchanged, nothing to update");
        return true;
    }

    std::string stationsJson;
    if (!downloadAndValidateStations(stationsJson)) {
        return false;
    }

    if (!stageAndSwapFiles(mRemoteManifestJson, stationsJson)) {
        return false;
    }

    if (!mStationRepository.load()) {
        ESP_LOGW(Tag, "Loading new stations failed");
        return false;
    }

    (void)mUiEventQueue.post(common::CurrentStationChangedEvent{});
    return true;
}

bool SyncStationsCommand::manifestsDifferent(bool& outDifferent) {
    outDifferent = false;

    common::ManifestData remoteManifest{};
    if (!mJsonParser.parseManifest(mRemoteManifestJson, remoteManifest)) {
        ESP_LOGW(Tag, "Remote manifest is invalid");
        return false;
    }

    std::string localManifestJson;
    if (!mFileSystem.readFile(ManifestPath, localManifestJson)) {
        ESP_LOGI(Tag, "Local manifest is missing, update needed");
        outDifferent = true;
        return true;
    }

    common::ManifestData localManifest{};
    if (!mJsonParser.parseManifest(localManifestJson, localManifest)) {
        ESP_LOGI(Tag, "Local manifest invalid, update needed");
        outDifferent = true;
        return true;
    }

    outDifferent = (localManifest.version != remoteManifest.version);
    return true;
}

bool SyncStationsCommand::downloadAndValidateStations(std::string& outStationsJson) {
    outStationsJson.clear();
    if (!mHttpClient.download(StationsUrl, outStationsJson, DownloadTimeoutMs)) {
        ESP_LOGW(Tag, "Failed to download stations.json");
        return false;
    }

    std::vector<common::StationData> parsed;
    if (!mJsonParser.parseStations(outStationsJson, parsed) || parsed.empty()) {
        ESP_LOGW(Tag, "Downloaded stations.json is invalid");
        return false;
    }

    return true;
}

bool SyncStationsCommand::stageAndSwapFiles(const std::string& manifestJson,
                                            const std::string& stationsJson) {
    if (!mFileSystem.writeFile(StationsTmpPath, stationsJson)) {
        ESP_LOGW(Tag, "Failed to stage stations tmp file");
        return false;
    }

    if (!mFileSystem.writeFile(ManifestTmpPath, manifestJson)) {
        ESP_LOGW(Tag, "Failed to stage manifest tmp file");
        (void)mFileSystem.removeFile(StationsTmpPath);
        return false;
    }

    if (!replaceFileAtomically(StationsPath, StationsTmpPath, StationsBackupPath)) {
        ESP_LOGW(Tag, "Failed to swap stations file atomically");
        (void)mFileSystem.removeFile(StationsTmpPath);
        (void)mFileSystem.removeFile(ManifestTmpPath);
        return false;
    }

    if (!replaceFileAtomically(ManifestPath, ManifestTmpPath, ManifestBackupPath)) {
        ESP_LOGW(Tag, "Failed to swap manifest file atomically");
        (void)mFileSystem.removeFile(ManifestTmpPath);
        return false;
    }

    return true;
}

bool SyncStationsCommand::replaceFileAtomically(const std::string& livePath,
                                                const std::string& tmpPath,
                                                const std::string& backupPath) {
    if (livePath.empty() || tmpPath.empty() || backupPath.empty()) {
        return false;
    }

    if (livePath == tmpPath || livePath == backupPath || tmpPath == backupPath) {
        return false;
    }

    if (!mFileSystem.exists(tmpPath)) {
        return false;
    }

    if (!mFileSystem.removeFile(backupPath)) {
        return false;
    }

    const bool hadLiveFile = mFileSystem.exists(livePath);
    if (hadLiveFile && !mFileSystem.renameFile(livePath, backupPath)) {
        return false;
    }

    if (!mFileSystem.renameFile(tmpPath, livePath)) {
        if (hadLiveFile) {
            (void)mFileSystem.renameFile(backupPath, livePath);
        }
        return false;
    }

    if (hadLiveFile && !mFileSystem.removeFile(backupPath)) {
        return false;
    }

    return true;
}

void SyncStationsCommand::finish() {
    (void)mUiEventQueue.post(common::SwitchToMainScreenEvent{});
    mFinished = true;
}

}  // namespace core::commands
