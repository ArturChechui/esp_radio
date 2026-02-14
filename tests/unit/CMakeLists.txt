cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(esp_radio_unit_tests)

set(COMPONENTS_DIR ${CMAKE_SOURCE_DIR}/../../components)

find_package(GTest REQUIRED)

enable_testing()

# Coverage build option
option(ENABLE_COVERAGE "Enable coverage" ON)

if(ENABLE_COVERAGE)
  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)
endif()

include(adapters/CMakeLists.txt)
include(services/CMakeLists.txt)
include(core/CMakeLists.txt)
