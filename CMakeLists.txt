cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")

option(ENABLE_LOGS "Enable ESP-IDF logging" ON)

if(NOT ENABLE_LOGS)
  message(STATUS "Build Mode: SILENT (Applying sdkconfig.silent)")
  list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.silent")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp_radio)

# add_compile_options(-g0) # No debug info add_compile_options(-Os) # Optimize
# for size
