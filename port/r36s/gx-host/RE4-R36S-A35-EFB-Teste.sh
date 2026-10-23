#!/bin/sh
# R36S DarkOSRE: A/B test Cortex-A35 optimized renderer at normal/half EFB.
# Core, data, working stable binary and GPU swap mode 5 are not modified.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
RES="${RE4_R36S_TEST_RES:-half}"
case "$RES" in
  half)
    BIN=r36s_game_a35_half
    EFBSIZE=320x264
    HALF=1
    ;;
  full)
    BIN=r36s_game_a35_full
    EFBSIZE=640x528
    HALF=0
    ;;
  *)
    echo "Invalid RE4_R36S_TEST_RES=$RES (half or full)"
    exit 2
    ;;
esac
DURATION="${RE4_R36S_SECONDS:-180}"
case "$DURATION" in *[!0-9]*|"") echo "Invalid duration";exit 2;; esac
if [ "$DURATION" -lt 15 ] || [ "$DURATION" -gt 600 ]; then echo "Duration must be 15..600";exit 2;fi
CACHE="$CORE/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
LOG="$CORE/diagnostico_a35_efb_${EFBSIZE}_darkos.txt"
(
  echo "==== CUBEEVIL4 R36S CORTEX-A35 -O3 LTO REAL EFB RENDER TEST ===="
  date
  uname -a
  free -m
  echo "A35_TEST=1 EFB_RENDER_SIZE_REQUESTED=$EFBSIZE HALF_EFB=$HALF"
  echo "NOTE=output is upscaled to the R36S display; no game geometry is discarded"
  echo "NOTE=do not assume 4x FPS: draw-call count and core decoding remain the same"
  echo "COMPILER=-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto -ffp-contract=off"
  echo "SHADER_CACHE_FILES_BEFORE=$(find "$CACHE" -type f -name '*.bin' | wc -l)"
  for entry in "$BIN" re4_coreproc libre4_main.so libre4_platform.so; do
    if [ ! -f "$CORE/$entry" ];then echo "MISSING_FILE=$entry";exit 127;fi
  done
  if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA" ];then
    echo "MISSING_GAME_DATA_OR_MODULES=1";exit 127
  fi
  chmod +x "$CORE/$BIN" "$CORE/re4_coreproc" 2>/dev/null || :
  export SDL_VIDEODRIVER=kmsdrm
  export SDL_VIDEO_EGL_DRIVER=libEGL.so
  export RE4_GX_BASEVERTEX=0
  export RE4_R36S_SWAP_MODE=5
  export RE4_R36S_SKIP_DOF=1
  export RE4_R36S_LIGHTS_MAX=2
  export RE4_R36S_SKIP_FOG=1
  export RE4_R36S_FAR_Z=0
  unset RE4_R36S_UNSAFE_FAR_CULL
  export RE4_R36S_HALF_EFB="$HALF"
  export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
  export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  cd "$CORE" || exit 2
  "./$BIN" "$CORE" "$CORE/modules" "$DATA" "$DURATION"
  rc=$?
  echo "SHADER_CACHE_FILES_AFTER=$(find "$CACHE" -type f -name '*.bin' | wc -l)"
  du -sk "$CACHE"
  printf 'A35_EFB_TEST_EXIT_CODE=%s\n' "$rc"
  exit "$rc"
) > "$LOG" 2>&1
rc=$?
printf 'A35 EFB %s test finished (code %s)\nLog: %s\n' "$EFBSIZE" "$rc" "$LOG"
sleep 3
exit "$rc"
