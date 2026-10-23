#!/bin/sh
# Resident Evil 4 on R36S - experimental fixed-scale quality and RAM profiling.
# Changes EFB pixels, not game-world visibility or model geometry.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
SCALE="${RE4_R36S_RENDER_SCALE:-0.75}"
case "$SCALE" in
  0.75) SIZE="480x396" ;;
  0.5) SIZE="320x264" ;;
  0.375) SIZE="240x198" ;;
  *) echo "RE4_R36S_RENDER_SCALE must be 0.75, 0.5 or 0.375";exit 2 ;;
esac
DURATION="${RE4_R36S_SECONDS:-180}"
case "$DURATION" in *[!0-9]*|"") echo "Invalid session duration";exit 2;;esac
if [ "$DURATION" -lt 15 ] || [ "$DURATION" -gt 600 ]; then
  echo "RE4_R36S_SECONDS must be 15..600";exit 2
fi
CACHE="$CORE/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
LOG="$CORE/diagnostico_escala_${SIZE}_a35_darkos.txt"
(
  echo "==== R36S COMPARE INTERNAL EFB PIXEL COUNTS AND RAM ===="
  date
  uname -a
  free -m
  echo "SCALE_REQUESTED=$SCALE INTERNAL_SIZE_REQUESTED=$SIZE"
  echo "COMPILER=-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto"
  echo "QUALITY=2_dynamic_lights DOF=off FOG=off DISTANCE_CULL=off"
  echo "SWAP=vsync1_glFinish_before_swap"
  echo "NOTE=If visual artefacts appear, immediately use existing stable launcher"
  for entry in r36s_game_a35_scalable re4_coreproc libre4_main.so libre4_platform.so; do
    if [ ! -f "$CORE/$entry" ]; then echo "MISSING_FILE=$entry";exit 127;fi
  done
  if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA" ]; then
    echo "MISSING_GAME_DATA_OR_MODULES=1";exit 127
  fi
  chmod +x "$CORE/r36s_game_a35_scalable" "$CORE/re4_coreproc" 2>/dev/null || :
  export SDL_VIDEODRIVER=kmsdrm
  export SDL_VIDEO_EGL_DRIVER=libEGL.so
  export RE4_GX_BASEVERTEX=0
  export RE4_R36S_SWAP_MODE=5
  export RE4_R36S_SKIP_DOF=1
  export RE4_R36S_LIGHTS_MAX=2
  export RE4_R36S_SKIP_FOG=1
  export RE4_R36S_FAR_Z=0
  unset RE4_R36S_UNSAFE_FAR_CULL
  export RE4_R36S_RENDER_SCALE="$SCALE"
  export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
  export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  cd "$CORE" || exit 2
  ./r36s_game_a35_scalable "$CORE" "$CORE/modules" "$DATA" "$DURATION"
  rc=$?
  printf "QUALITY_SCALE_EXIT_CODE=%s\n" "$rc"
  exit "$rc"
) > "$LOG" 2>&1
rc=$?
printf "R36S framebuffer %s test finished (code %s)\nLog: %s\n" "$SIZE" "$rc" "$LOG"
sleep 3
exit "$rc"
