#!/bin/sh
# Offline-safe smoke + RE4NX synthetic suite; no proprietary games or keys.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
: "${CLOUDLAB_BUILD:=$ROOT/.cloud-build}"
mkdir -p "$CLOUDLAB_BUILD"
if [ "${1:-}" != "" ]; then
 python3 "$HERE/verify_profile.py" "$1"
 python3 "$HERE/profile_report.py" "$1" --output "$CLOUDLAB_BUILD/hardware-profile.json"
else
 echo 'TARGET_PROFILE=CURATED_SNAPSHOT; raw private collector archive not required in CI'
 cp "$ROOT/docs/R36S-DARKOSRE-REAL-PROFILE.json" "$CLOUDLAB_BUILD/hardware-profile.json"
fi
python3 - "$CLOUDLAB_BUILD/hardware-profile.json" <<'PYPROFILE'
import json,sys
p=json.load(open(sys.argv[1]))
assert p['arch']=='aarch64' and p['cpu']['cores']==4
assert p['cpu']['page_size_bytes']==4096
assert p['memory']['reported_total_mib']==895
assert p['memory']['recommended_rss_budget_mib']==512
print('HARDWARE_BASELINE=PASS RAM_MIB=895 CPU=CORTEX_A35 GPU_TARGET=MALI_G31_PAGE_SIZE_4096')
print('REAL_MALI_HARDWARE_TESTS=REQUIRED_FOR_FINAL_FPS_VALIDATION')
PYPROFILE
python3 "$HERE/tests/test_bundle.py"
cmake -S "$HERE" -B "$CLOUDLAB_BUILD/smoke" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build "$CLOUDLAB_BUILD/smoke" --parallel 2
if [ "${CLOUDLAB_REQUIRE_GLES3:-0}" = "1" ] && [ ! -x "$CLOUDLAB_BUILD/smoke/cloud-gles3-smoke" ]; then
 echo 'CLOUD_GLES3=FAIL required software EGL/GLES3 target is not available' >&2
 exit 1
fi
EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 \
 ctest --test-dir "$CLOUDLAB_BUILD/smoke" --output-on-failure
if [ -d "$ROOT/re4nx" ]; then
 echo 'RE4NX_SOURCE=FOUND; testing the actual Phase 1-3 synthetic source'
 cmake -S "$ROOT/re4nx" -B "$CLOUDLAB_BUILD/re4nx" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
 cmake --build "$CLOUDLAB_BUILD/re4nx" --parallel 2
 ctest --test-dir "$CLOUDLAB_BUILD/re4nx" --output-on-failure
 if [ -x "$CLOUDLAB_BUILD/re4nx/re4nx-native-host" ]; then
  python3 "$ROOT/re4nx/tests/test_phase2.py" "$CLOUDLAB_BUILD/re4nx/re4nx-native-host"
  python3 "$ROOT/re4nx/tests/test_phase3.py" "$CLOUDLAB_BUILD/re4nx/re4nx-phase3-host"
 else
  echo 'HOST_ONLY_NEGATIVE_TESTS=SKIPPED_ON_ARM64; native Phase2/3 regressions run below'
 fi
 (cd "$ROOT/re4nx" && sh tools/build-native-phase2-aarch64.sh && sh tools/build-native-phase3-aarch64.sh)
 python3 "$HERE/check_abi.py" "$ROOT/re4nx/re4nx-native-aarch64" "$ROOT/re4nx/re4nx-hle-aarch64"
 if [ "$(uname -m)" = "aarch64" ]; then
  "$ROOT/re4nx/re4nx-native-aarch64" --synthetic "$ROOT/re4nx/samples/synthetic-aarch64.nso"
  "$ROOT/re4nx/re4nx-native-aarch64" --memory-probe
  "$ROOT/re4nx/re4nx-hle-aarch64" --selftest
  "$ROOT/re4nx/re4nx-hle-aarch64" --stress
  python3 "$ROOT/re4nx/tests/test_phase2.py" "$ROOT/re4nx/re4nx-native-aarch64"
  python3 "$ROOT/re4nx/tests/test_phase3.py" "$ROOT/re4nx/re4nx-hle-aarch64"
  echo 'RE4NX_ARM64_NATIVE_TESTS=PASS'
 else
  if [ "${CLOUDLAB_STRICT_ARM64:-0}" = "1" ]; then
   echo 'RE4NX_ARM64_NATIVE_TESTS=FAIL unexpected non-AArch64 runner' >&2
   exit 1
  fi
  echo 'RE4NX_ARM64_NATIVE_TESTS=SKIPPED_NON_AARCH64; use ubuntu-24.04-arm or QEMU'
 fi
fi
echo 'DARKOSRE_CLOUDLAB=PASS (functional tests only; Mali-G31 performance not reproduced)'
