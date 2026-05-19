/**
 * @file IJsonParser.hpp
 * @brief Interface for JSON parsing utilities.
 *
 * This file defines the IJsonParser interface, which is used to deserialize
 * radio station lists and system manifests from JSON strings.
 */

#pragma once

#include <string>
#include <vector>

#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {

/**
 * @class IJsonParser
 * @brief Abstract interface for parsing system data from JSON.
 *
 * Implementations of this interface wrap a JSON library to provide
 * specialized parsing for internal data structures like StationData
 * and ManifestData.
 */
class IJsonParser {
   public:
    /** @brief Virtual destructor for safe interface cleanup. */
    virtual ~IJsonParser() = default;

    /**
     * @brief Parses a list of stations from a serialized JSON string.
     * @param serialized The raw JSON string containing station information.
     * @param outStations A vector to be populated with the parsed StationData objects.
     * @return true if the JSON was valid and the stations were successfully parsed.
     */
    virtual bool parseStations(const std::string& serialized,
                               std::vector<common::StationData>& outStations) = 0;

    /**
     * @brief Parses system manifest data from a serialized JSON string.
     * @param serialized The raw JSON string containing manifest information.
     * @param outManifest The structure to be populated with parsed manifest data.
     * @return true if the manifest was successfully parsed.
     */
    virtual bool parseManifest(const std::string& serialized,
                               common::ManifestData& outManifest) = 0;
};
}  // namespace common
