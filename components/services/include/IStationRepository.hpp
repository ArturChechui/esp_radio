/**
 * @file IStationRepository.hpp
 * @brief Interface definition for the radio station database repository.
 *
 * This file defines the abstract interface for managing a collection of
 * radio stations, including loading data from storage and navigating
 * the station list.
 */

#pragma once

#include <vector>

#include "Types.hpp"

/**
 * @namespace services
 * @brief Contains business logic services that coordinate hardware and application state.
 */
namespace services {

/**
 * @class IStationRepository
 * @brief Abstract interface for a station data repository.
 *
 * This interface provides a standardized way to access and navigate the
 * list of available radio stations. It abstracts the underlying storage
 * mechanism (e.g., JSON files on LittleFS).
 */
class IStationRepository {
   public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IStationRepository() = default;

    /**
     * @brief Initializes the repository and prepares internal data structures.
     * @return true if initialization was successful, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Loads station data from persistent storage into memory.
     * @return true if data was loaded successfully, false if the storage is empty or corrupted.
     */
    virtual bool load() = 0;

    /**
     * @brief Retrieves the entire list of stations currently in the repository.
     * @return A constant reference to a vector of StationData structures.
     */
    virtual const std::vector<common::StationData> &getStations() const = 0;

    /**
     * @brief Advances to the next station in the list and returns its data.
     * * If the end of the list is reached, implementations typically wrap back
     * to the first station.
     * * @return A constant reference to the StationData of the new current station.
     */
    virtual const common::StationData &nextStation() = 0;

    /**
     * @brief Moves to the previous station in the list and returns its data.
     * * If the beginning of the list is reached, implementations typically wrap
     * to the last station.
     * * @return A constant reference to the StationData of the new current station.
     */
    virtual const common::StationData &prevStation() = 0;

    /**
     * @brief Retrieves the data for the station currently selected in the repository.
     * @return A constant reference to the current StationData.
     */
    virtual const common::StationData &currentStation() const = 0;
};

}  // namespace services
