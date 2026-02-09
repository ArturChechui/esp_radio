add_executable(test_adapters ${CMAKE_SOURCE_DIR}/adapters/DisplayTest.cpp
                             ${COMPONENTS_DIR}/adapters/src/Display.cpp)

target_include_directories(
  test_adapters
  PRIVATE ${CMAKE_SOURCE_DIR}/stubs ${COMPONENTS_DIR}/adapters/include
          ${COMPONENTS_DIR}/common/include ${COMPONENTS_DIR}/adapters/mock)

target_link_libraries(test_adapters GTest::GTest GTest::Main GTest::gmock
                      GTest::gmock_main)

gtest_discover_tests(test_adapters)
