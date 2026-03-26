#pragma once

#include <string>
#include <vector>

#include "IJsonParser.hpp"
#include "Types.hpp"

namespace common {
class JsonParser final : public IJsonParser {
   public:
    // Expects JSON array of objects: [{ "id": "...", "name": "...", "url": "..." }, ...]
    bool parseStations(const std::string& serialized,
                       std::vector<common::StationData>& outStations) override;

    bool parseManifest(const std::string& serialized, common::ManifestData& outManifest) override;
};
}  // namespace common
