cmake_minimum_required(VERSION 3.16)

project(esp_radio_unit_tests)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
include("${REPO_ROOT}/cmake/common.cmake")
set(COMPONENTS_DIR "${REPO_ROOT}/components")

find_package(GTest REQUIRED)
enable_testing()

option(ENABLE_COVERAGE "Enable coverage" ON)

if(ENABLE_COVERAGE)
  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)
endif()

include(adapters/CMakeLists.txt)
include(services/CMakeLists.txt)
include(core/CMakeLists.txt)
