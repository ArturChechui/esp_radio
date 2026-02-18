add_executable(
  test_services
  ${CMAKE_SOURCE_DIR}/services/UiServiceTest.cpp
  ${CMAKE_SOURCE_DIR}/services/StationRepositoryTest.cpp
  ${CMAKE_SOURCE_DIR}/services/PlayerServiceTest.cpp
  ${CMAKE_SOURCE_DIR}/services/InputServiceTest.cpp
  ${CMAKE_SOURCE_DIR}/services/SensorServiceTest.cpp
  ${COMPONENTS_DIR}/services/src/UiService.cpp
  ${COMPONENTS_DIR}/services/src/StationRepository.cpp
  ${COMPONENTS_DIR}/services/src/PlayerService.cpp
  ${COMPONENTS_DIR}/services/src/InputService.cpp
  ${COMPONENTS_DIR}/services/src/SensorService.cpp
  ${COMPONENTS_DIR}/common/src/LockGuard.cpp
  ${COMPONENTS_DIR}/common/mock/FakeMutex.cpp)

target_include_directories(
  test_services
  PRIVATE ${CMAKE_SOURCE_DIR}/stubs
          ${COMPONENTS_DIR}/services/include
          ${COMPONENTS_DIR}/services/mock
          ${COMPONENTS_DIR}/common/include
          ${COMPONENTS_DIR}/common/mock
          ${COMPONENTS_DIR}/adapters/include
          ${COMPONENTS_DIR}/adapters/mock)

target_compile_definitions(test_services PUBLIC UNIT_TESTS)

target_link_libraries(test_services GTest::GTest GTest::Main GTest::gmock
                      GTest::gmock_main)

gtest_discover_tests(test_services)
