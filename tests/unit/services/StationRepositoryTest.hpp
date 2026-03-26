#pragma once

#include "gtest/gtest.h"
#include "MockFileSystem.hpp"
#include "MockJsonParser.hpp"
#include "MockPersistentStorage.hpp"
#include "StationRepository.hpp"

class StationRepositoryTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;
    void initSuccess();

    std::unique_ptr<adapters::MockPersistentStorage> mockPersistentStorage;
    std::unique_ptr<adapters::MockFileSystem> mockFileSystem;
    std::unique_ptr<common::MockJsonParser> mockParser;

    std::unique_ptr<services::StationRepository> stationRepository;
};
