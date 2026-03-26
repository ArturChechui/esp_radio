#pragma once

#include <vector>

#include "IStationRepository.hpp"
#include "Mutex.hpp"
#include "Types.hpp"
namespace common {
class IJsonParser;
}  // namespace common
namespace adapters {
class IPersistentStorage;
class IFileSystem;
}  // namespace adapters

namespace services {
class StationRepository : public IStationRepository {
   public:
    explicit StationRepository(adapters::IPersistentStorage &persistentStorage,
                               adapters::IFileSystem &fileSystem, common::IJsonParser &parser);
    ~StationRepository() override = default;

    bool init() override;
    bool load() override;
    const std::vector<common::StationData> &getStations() const override;
    const common::StationData &nextStation() override;
    const common::StationData &prevStation() override;
    const common::StationData &currentStation() const override;

   private:
    bool refreshStationsFromFile();

    adapters::IPersistentStorage &mPersistentStorage;
    adapters::IFileSystem &mFileSystem;
    common::IJsonParser &mParser;
    std::vector<common::StationData> mStations;
    bool mInitialized;
    uint32_t mCurrentStationIdx;
    mutable common::Mutex mMutex;
};
}  // namespace services
