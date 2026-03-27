#include "StationRepositoryTest.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

void StationRepositoryTest::SetUp() {
    mockPersistentStorage = std::make_unique<adapters::MockPersistentStorage>();
    mockFileSystem = std::make_unique<adapters::MockFileSystem>();
    mockParser = std::make_unique<common::MockJsonParser>();

    stationRepository = std::make_unique<services::StationRepository>(*mockPersistentStorage,
                                                                      *mockFileSystem, *mockParser);
}

void StationRepositoryTest::TearDown() {
    stationRepository.reset();

    mockPersistentStorage.reset();
    mockFileSystem.reset();
    mockParser.reset();
}

void StationRepositoryTest::initSuccess() {
    std::vector<common::StationData> stations = {
        {"hitfm_1", "Hit FM", "https://online.hitfm.ua/HitFM"},
        {"hitfmhd_2", "HitHD", "https://online.hitfm.ua/HitFM_HD"},
        {"kissfm_3", "KissFM", "http://online.kissfm.ua/KissFM"},
        {"kissfmhd_4", "KissHD", "https://online.kissfm.ua/KissFM_HD"},
        {"luxfmhd_5", "LuxHD", "http://icecast.luxnet.ua/luxlviv_hd"},
        {"relax_6", "Relax", "https://online.radiorelax.ua/RadioRelax_Ukr"},
        {"pyatnica_7", "Friday", "https://cast.mediaonline.net.ua/radiopyatnica"},
        {"nasheradio_8", "Nashe", "http://online.nasheradio.ua/NasheRadio"}};

    EXPECT_CALL(*mockFileSystem, readFile("stations.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            outData = "stations in a json format";
            return true;
        });

    EXPECT_CALL(*mockParser, parseStations(_, _))
        .WillOnce([stations](const std::string& serialized,
                             std::vector<common::StationData>& outStations) {
            outStations = stations;
            return true;
        });

    EXPECT_CALL(*mockPersistentStorage, getU32("station_idx", _))
        .WillOnce([](const std::string& key, uint32_t& out) {
            out = 0;
            return true;
        });

    EXPECT_TRUE(stationRepository->init());
    EXPECT_EQ(stations.size(), stationRepository->getStations().size());
}

TEST_F(StationRepositoryTest, tc01_init) {
    initSuccess();
}

TEST_F(StationRepositoryTest, tc02_nextStation) {
    initSuccess();

    const auto& exp = stationRepository->getStations().at(1);
    EXPECT_CALL(*mockPersistentStorage, setU32("station_idx", 1)).WillOnce(Return(true));
    const auto& station = stationRepository->nextStation();

    EXPECT_EQ(station.id, exp.id);
}

TEST_F(StationRepositoryTest, tc03_prevStation) {
    initSuccess();

    const auto& exp = stationRepository->getStations().at(7);
    EXPECT_CALL(*mockPersistentStorage, setU32("station_idx", 7)).WillOnce(Return(true));
    const auto& station = stationRepository->prevStation();

    EXPECT_EQ(station.id, exp.id);
}

TEST_F(StationRepositoryTest, tc04_currentStation) {
    initSuccess();

    const auto& exp = stationRepository->getStations().at(0);
    const auto& station = stationRepository->currentStation();

    EXPECT_EQ(station.id, exp.id);
}

TEST_F(StationRepositoryTest, tc05_init_noStations) {
    EXPECT_CALL(*mockFileSystem, readFile("stations.json", _))
        .WillOnce([](const std::string& relativePath, std::string& outData) {
            outData = "";
            return false;
        });

    // still returns true since it can be fixed by uploading new stations instead of blocking the
    // app and wait for a reflash
    EXPECT_TRUE(stationRepository->init());
    EXPECT_EQ(0, stationRepository->getStations().size());
}

TEST_F(StationRepositoryTest, tc06_load_success_updatesStations) {
    initSuccess();

    std::vector<common::StationData> updatedStations = {
        {"new1", "New One", "https://example.com/new1"},
        {"new2", "New Two", "https://example.com/new2"}};

    EXPECT_CALL(*mockFileSystem, readFile("stations.json", _))
        .WillOnce([](const std::string&, std::string& outData) {
            outData = "new stations json";
            return true;
        });
    EXPECT_CALL(*mockParser, parseStations(_, _))
        .WillOnce(
            [updatedStations](const std::string&, std::vector<common::StationData>& outStations) {
                outStations = updatedStations;
                return true;
            });
    EXPECT_CALL(*mockPersistentStorage, setU32("station_idx", 0)).WillOnce(Return(true));

    EXPECT_TRUE(stationRepository->load());
    EXPECT_EQ(2, stationRepository->getStations().size());
    EXPECT_EQ("new1", stationRepository->currentStation().id);
}

TEST_F(StationRepositoryTest, tc07_reload_fail_keepsPreviousStations) {
    initSuccess();
    const std::string beforeId = stationRepository->currentStation().id;

    EXPECT_CALL(*mockFileSystem, readFile("stations.json", _))
        .WillOnce([](const std::string&, std::string& outData) {
            outData = "broken json";
            return true;
        });
    EXPECT_CALL(*mockParser, parseStations(_, _)).WillOnce(Return(false));

    EXPECT_FALSE(stationRepository->load());
    EXPECT_EQ(beforeId, stationRepository->currentStation().id);
}
