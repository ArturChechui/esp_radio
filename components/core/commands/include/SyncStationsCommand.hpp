#pragma once

#include <string>

#include "ICommand.hpp"

namespace adapters {
class IHttpClient;
class IFileSystem;
}  // namespace adapters

namespace common {
class IEventQueue;
class IJsonParser;
}  // namespace common

namespace services {
class IPlayerService;
class IStationRepository;
class IWifiService;
}  // namespace services

namespace core::commands {

class SyncStationsCommand : public ICommand {
   public:
    SyncStationsCommand(services::IPlayerService& playerService,
                        services::IWifiService& wifiService, adapters::IHttpClient& httpClient,
                        adapters::IFileSystem& fileSystem, common::IJsonParser& jsonParser,
                        services::IStationRepository& stationRepository,
                        common::IEventQueue& uiEventQueue);
    ~SyncStationsCommand() override = default;

    void handle(const common::AppEvent& e) override;
    bool isFinished() override;

   private:
    bool stopPlaybackIfStarted();
    void onPlaybackStatus(common::PlaybackStatus s);
    bool runSync();
    bool manifestsDifferent(bool& outDifferent);
    bool downloadAndValidateStations(std::string& outStationsJson);
    bool stageAndSwapFiles(const std::string& manifestJson, const std::string& stationsJson);
    // TODO: rename the func?
    bool replaceFileAtomically(const std::string& livePath, const std::string& tmpPath,
                               const std::string& backupPath);
    void finish();

    services::IPlayerService& mPlayerService;
    services::IWifiService& mWifiService;
    adapters::IHttpClient& mHttpClient;
    adapters::IFileSystem& mFileSystem;
    common::IJsonParser& mJsonParser;
    services::IStationRepository& mStationRepository;
    common::IEventQueue& mUiEventQueue;

    bool mStarted;
    bool mFinished;
    std::string mRemoteManifestJson;
};

}  // namespace core::commands
