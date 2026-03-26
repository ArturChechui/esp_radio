#pragma once

#include <gmock/gmock.h>

#include "IJsonParser.hpp"
#include "Types.hpp"

namespace common {
class MockJsonParser : public IJsonParser {
   public:
    MOCK_METHOD(bool, parseStations,
                (const std::string& serialized, std::vector<common::StationData>& outStations),
                (override));
    MOCK_METHOD(bool, parseManifest,
                (const std::string& serialized, common::ManifestData& outManifest), (override));
};
}  // namespace common
