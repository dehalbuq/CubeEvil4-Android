// Host-only tests for the opt-in R36S graphics policy (no GL context needed).
#include "gx_r36s_profile.h"

#include <cassert>
#include <cstdlib>

int main() {
  unsetenv("RE4_R36S_LOW_MEM");
  assert(!re4gx::r36s::LowMemory());
  assert(re4gx::r36s::LimitHalves(6) == 6);
  assert(re4gx::r36s::LimitSamples(4) == 4);
  assert(re4gx::r36s::LimitAnisotropy(8.0f) == 8.0f);
  assert(re4gx::r36s::PostEffect(true));

  setenv("RE4_R36S_LOW_MEM", "0", 1);
  assert(!re4gx::r36s::LowMemory());
  setenv("RE4_R36S_LOW_MEM", "10", 1);
  assert(!re4gx::r36s::LowMemory());

  setenv("RE4_R36S_LOW_MEM", "1", 1);
  assert(re4gx::r36s::LowMemory());
  assert(re4gx::r36s::LimitHalves(6) == 2);
  assert(re4gx::r36s::LimitHalves(2) == 2);
  assert(re4gx::r36s::LimitSamples(4) == 1);
  assert(re4gx::r36s::LimitAnisotropy(16.0f) == 1.0f);
  assert(!re4gx::r36s::PostEffect(true));
  assert(!re4gx::r36s::PostEffect(false));
  assert(re4gx::r36s::kProgramLimit >= re4gx::r36s::kMergedProgramLimit);
  unsetenv("RE4_R36S_LOW_MEM");
  return 0;
}
