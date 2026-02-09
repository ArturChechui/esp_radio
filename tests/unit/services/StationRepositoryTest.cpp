#include "StationRepositoryTest.hpp"

void StationRepositoryTest::SetUp() {
    stationRepository = std::make_unique<services::StationRepository>();
}

void StationRepositoryTest::TearDown() {
    stationRepository.reset();
}

TEST_F(StationRepositoryTest, tc01_getStations_returnsInitializedStations) {
    // Arrange
    std::vector<common::StationData> expectedStations = {
        {"hitfm_hd_1", "HitFM_HD", "https://online.hitfm.ua/HitFM_HD"},
        {"hitfm_2", "HitFM", "https://online.hitfm.ua/HitFM"},
        {"radio1_3", "Radio1", "http://play.global.audio/radio164"},
        {"caroline_4", "Caroline", "https://stream.radiocaroline.net/;"},
        {"luxfmhd_5", "LuxFM_HD", "http://icecast.luxnet.ua/luxlviv_hd"},
        {"nasheradio_6", "NasheRadio", "http://online.nasheradio.ua/NasheRadio"}};

    // Act
    stationRepository->init();
    const auto& stations = stationRepository->getStations();

    // Expect
    // TODO: create operator== for StationData and use EXPECT_EQ directly
    EXPECT_EQ(expectedStations.size(), stations.size());
}
