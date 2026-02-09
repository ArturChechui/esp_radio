#pragma once

#include <vector>

#include "IStationRepository.hpp"
#include "Types.hpp"

namespace services {

class StationRepository : public IStationRepository {
   public:
    explicit StationRepository();
    ~StationRepository() override = default;

    bool init() override;
    const std::vector<common::StationData> &getStations() const override;
    const common::StationData &nextStation() override;
    const common::StationData &prevStation() override;
    const common::StationData &currentStation() const override;

   private:
    std::vector<common::StationData> mStations;
    bool mInitialized;
    uint32_t mCurrentStationIdx;
};

}  // namespace services
