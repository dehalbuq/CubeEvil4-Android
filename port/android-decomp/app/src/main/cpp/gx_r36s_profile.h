// Optional low-resource presentation profile for a Linux R36S frontend.
// This does not change RE4 gameplay clocks, AI, collision, or original game assets.
// Enabled ONLY when the caller starts the process with RE4_R36S_LOW_MEM=1.
#pragma once

#include <cstddef>
#include <cstdlib>

namespace re4gx {
namespace r36s {

inline bool LowMemory() {
  const char *value = std::getenv("RE4_R36S_LOW_MEM");
  return value && value[0] == '1' && value[1] == '\0';
}

// The GameCube EFB stays at 1x (640x528); never allocate 2x/3x targets.
inline int LimitHalves(int requested) { return LowMemory() && requested > 2 ? 2 : requested; }
inline int LimitSamples(int requested) { return LowMemory() ? 1 : requested; }
inline float LimitAnisotropy(float requested) { return LowMemory() ? 1.0f : requested; }
inline bool PostEffect(bool requested) { return !LowMemory() && requested; }

// Driver shader programs can own significant RAM on a 1 GiB machine. Avoid
// large in-RAM program maps; disk caches still remain available when configured.
// Limits are experimental; they may cause shader-cache reload stutter.
constexpr std::size_t kProgramLimit = 256;
constexpr std::size_t kMergedProgramLimit = 64;

}  // namespace r36s
}  // namespace re4gx
