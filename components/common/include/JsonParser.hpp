/**
 * @file JsonParser.hpp
 * @brief Concrete implementation of the IJsonParser interface.
 *
 * This file contains the JsonParser class, which handles the deserialization
 * of radio station lists and system manifest data using a JSON backend.
 */

#pragma once

#include <string>
#include <vector>

#include "IJsonParser.hpp"
#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class JsonParser
 * @brief A JSON parser for radio station and manifest data.
 *
 * This class provides methods to convert serialized JSON strings into
 * structured C++ objects used by the system's repositories and update services.
 */
class JsonParser final : public IJsonParser {
   public:
    /**
     * @brief Parses a list of stations from a JSON array.
     * * @note Expects a JSON array of objects with the following format:
     * `[{ "id": "...", "name": "...", "url": "..." }, ...]`
     *
     * @param serialized The raw JSON string containing the station list.
     * @param outStations A vector to be populated with the parsed StationData.
     * @return true if the parsing was successful and all required fields were found.
     */
    bool parseStations(const std::string& serialized,
                       std::vector<common::StationData>& outStations) override;

    /**
     * @brief Parses system manifest data from a JSON object.
     *
     * This is typically used for checking software versions or remote configuration parameters.
     *
     * @param serialized The raw JSON string containing manifest info.
     * @param outManifest The structure to be populated with parsed data.
     * @return true if the manifest was successfully deserialized.
     */
    bool parseManifest(const std::string& serialized, common::ManifestData& outManifest) override;
};

}  // namespace common
