/**
 * @file StationRepository.hpp
 * @brief Implementation of the IStationRepository interface for station management.
 *
 * This file contains the StationRepository class, which manages a collection of
 * radio stations loaded from a JSON file and tracks the current selection
 * across reboots using persistent storage.
 */

#pragma once

#include <vector>

#include "IStationRepository.hpp"
#include "Mutex.hpp"
#include "Types.hpp"

/**
 * @namespace common
 * @brief Contains shared utility and messaging interfaces.
 */
namespace common {
class IJsonParser;
}  // namespace common

/**
 * @namespace adapters
 * @brief Contains hardware and system abstraction layer interfaces.
 */
namespace adapters {
class IPersistentStorage;
class IFileSystem;
}  // namespace adapters

/**
 * @namespace services
 * @brief Contains business logic service implementations.
 */
namespace services {

/**
 * @class StationRepository
 * @brief Concrete implementation of a station database with persistence.
 *
 * StationRepository handles the loading of station lists from the filesystem
 * and uses PersistentStorage to remember which station was last selected.
 * All access to the internal station list and index is protected by a mutex
 * to ensure thread safety.
 */
class StationRepository : public IStationRepository {
   public:
    /**
     * @brief Constructs a StationRepository with its required dependencies.
     * @param persistentStorage Storage adapter for saving/loading selection state.
     * @param fileSystem Filesystem adapter for reading station JSON files.
     * @param parser Utility for parsing station list data.
     */
    explicit StationRepository(adapters::IPersistentStorage &persistentStorage,
                               adapters::IFileSystem &fileSystem, common::IJsonParser &parser);

    /** @brief Default virtual destructor. */
    ~StationRepository() override = default;

    /**
     * @brief Initializes the repository and restores the last selected station index.
     * @return true if initialization and state restoration succeeded.
     */
    bool init() override;

    /**
     * @brief Loads the station list from the filesystem.
     * @return true if the stations were successfully loaded into memory.
     */
    bool load() override;

    /**
     * @brief Retrieves the full list of loaded stations.
     * @return A constant reference to the station data vector.
     */
    const std::vector<common::StationData> &getStations() const override;

    /**
     * @brief Advances to the next station and persists the new selection.
     * @return A constant reference to the newly selected station.
     */
    const common::StationData &nextStation() override;

    /**
     * @brief Moves to the previous station and persists the new selection.
     * @return A constant reference to the newly selected station.
     */
    const common::StationData &prevStation() override;

    /**
     * @brief Retrieves the station currently selected.
     * @return A constant reference to the active station data.
     */
    const common::StationData &currentStation() const override;

   private:
    /**
     * @brief Internal helper to reload station data from the JSON file.
     * @return true if the file was successfully read and parsed.
     */
    bool refreshStationsFromFile();

    adapters::IPersistentStorage &mPersistentStorage; /**< Reference to persistence adapter. */
    adapters::IFileSystem &mFileSystem;               /**< Reference to storage adapter. */
    common::IJsonParser &mParser;                     /**< Reference to JSON parsing utility. */
    std::vector<common::StationData> mStations;       /**< In-memory list of stations. */
    bool mInitialized;                                /**< Flag: indicates if init() succeeded. */
    uint32_t mCurrentStationIdx;                      /**< The index of the active station. */
    mutable common::Mutex mMutex;                     /**< Mutex for thread-safe list access. */
};

}  // namespace services
