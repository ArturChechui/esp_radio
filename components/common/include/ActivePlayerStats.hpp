#pragma once

#include "NullAudioBufferStats.hpp"

#ifdef USE_NULL_AUDIO_STATS
namespace common {
// Release builds: All calls collapse into inline no-ops
using ActivePlayerStats = NullAudioBufferStats;
}  // namespace common
#else
#include "AudioBufferStats.hpp"

namespace common {
// Debug/Profiling builds: Full metrics gathering
using ActivePlayerStats = AudioBufferStats;
}  // namespace common
#endif
