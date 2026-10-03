cmake_minimum_required(VERSION 3.16)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/common.cmake")

set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")

option(ENABLE_LOGS "Enable ESP-IDF logging" OFF)
if(NOT ENABLE_LOGS)
  message(STATUS "Build Mode: SILENT (Applying sdkconfig.silent)")
  list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.silent")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp_radio)
