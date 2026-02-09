cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(esp_radio_unit_tests)

set(COMPONENTS_DIR ${CMAKE_SOURCE_DIR}/../../components)

find_package(GTest REQUIRED)

enable_testing()

include(adapters/CMakeLists.txt)
include(services/CMakeLists.txt)
include(core/CMakeLists.txt)
