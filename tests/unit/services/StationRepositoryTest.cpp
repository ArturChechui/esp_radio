#include "StationRepositoryTest.hpp"

void StationRepositoryTest::SetUp() {
    stationRepository = std::make_unique<services::StationRepository>();
}

void StationRepositoryTest::TearDown() {
    stationRepository.reset();
}

TEST_F(StationRepositoryTest, tc01_getStations) {
    std::vector<common::StationData> expectedStations = {
        {"hitfm_1", "Hit FM", "https://online.hitfm.ua/HitFM"},
        {"hitfmhd_2", "HitHD", "https://online.hitfm.ua/HitFM_HD"},
        {"kissfm_3", "KissFM", "http://online.kissfm.ua/KissFM"},
        {"kissfmhd_4", "KissHD", "https://online.kissfm.ua/KissFM_HD"},
        {"luxfmhd_5", "LuxHD", "http://icecast.luxnet.ua/luxlviv_hd"},
        {"relax_6", "Relax", "https://online.radiorelax.ua/RadioRelax_Ukr"},
        {"pyatnica_7", "Friday", "https://cast.mediaonline.net.ua/radiopyatnica"},
        {"nasheradio_8", "Nashe", "http://online.nasheradio.ua/NasheRadio"}};

    EXPECT_TRUE(stationRepository->init());
    const auto& stations = stationRepository->getStations();

    EXPECT_EQ(expectedStations.size(), stations.size());
}

TEST_F(StationRepositoryTest, tc02_nextStation) {
    EXPECT_TRUE(stationRepository->init());
    const auto& exp = stationRepository->getStations().at(1);
    const auto& station = stationRepository->nextStation();

    EXPECT_EQ(station.id, exp.id);
}

TEST_F(StationRepositoryTest, tc03_prevStation) {
    EXPECT_TRUE(stationRepository->init());
    const auto& exp = stationRepository->getStations().at(7);
    const auto& station = stationRepository->prevStation();

    EXPECT_EQ(station.id, exp.id);
}

TEST_F(StationRepositoryTest, tc04_currentStation) {
    EXPECT_TRUE(stationRepository->init());
    const auto& exp = stationRepository->getStations().at(0);
    const auto& station = stationRepository->currentStation();

    EXPECT_EQ(station.id, exp.id);
}