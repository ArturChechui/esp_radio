#pragma once

#include <string>
#include <vector>

#include "Types.hpp"

namespace common {
class IJsonParser {
   public:
    virtual ~IJsonParser() = default;

    // Parses stations from serialized input into outStations.
    virtual bool parseStations(const std::string& serialized,
                               std::vector<common::StationData>& outStations) = 0;

    virtual bool parseManifest(const std::string& serialized,
                               common::ManifestData& outManifest) = 0;
};
}  // namespace common
