# Shared build options and compile definitions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_NULL_STATS "Build with NullAudioBufferStats (Release mode stats)" ON)

if(USE_NULL_STATS)
  add_compile_definitions(USE_NULL_AUDIO_STATS)
endif()
