#pragma once

#include <vector>

#include "Types.hpp"

namespace services {

class IStationRepository {
   public:
    virtual ~IStationRepository() = default;

    virtual bool init() = 0;
    virtual const std::vector<common::StationData> &getStations() const = 0;
    virtual const common::StationData &nextStation() = 0;
    virtual const common::StationData &prevStation() = 0;
    virtual const common::StationData &currentStation() const = 0;
};

}  // namespace services
