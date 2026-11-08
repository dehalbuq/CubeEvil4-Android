#!/bin/sh
# R36S Mali-G31: 480x396 same visual profile, compare GL geometry upload modes.
# Does not overwrite stable binaries nor touch original RE4 assets or saves.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
MODE="${RE4_R36S_GEOMETRY_MODE:-reuse}"
case "$MODE" in
  base) BIN=r36s_game_a35_geom_base ;;
  reuse) BIN=r36s_game_a35_geom_reuse ;;
  *) echo "INVALID_GEOMETRY_MODE=$MODE (base|reuse)"; exit 2 ;;
esac
DURATION="${RE4_R36S_SECONDS:-180}"
case "$DURATION" in *[!0-9]*|"") echo "INVALID_DURATION";exit 2;; esac
if [ "$DURATION" -lt 15 ] || [ "$DURATION" -gt 600 ]; then
  echo "DURATION_OUT_OF_RANGE=15..600";exit 2
fi
CACHE="$CORE/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
LOG="$CORE/diagnostico_a35_75_geometria_${MODE}_darkos.txt"
(
  echo "==== CUBEEVIL4 R36S GX VBO/IBO UPLOAD A/B ===="
  date
  uname -a
  free -m
  echo "MODE=$MODE BIN=$BIN"
  echo "EFB=480x396 GPU=Mali-G31 CPU=Cortex-A35"
  echo "BUILD=-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto"
  echo "VISUAL=2_lights DOF=OFF FOG=OFF NO_OBJECT_CULL NO_POLYGON_REDUCTION"
  echo "SWAP=5 glFinish_before_swap YES"
  echo "NOTE=GL_upload_timing_is_wall_clock_and_may_include_driver_wait"
  for item in "$BIN" re4_coreproc libre4_main.so libre4_platform.so;do
    if [ ! -f "$CORE/$item" ];then echo "MISSING_FILE=$item";exit 127;fi
  done
  if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA" ];then echo "MISSING_GAME_DATA_OR_MODULES";exit 127;fi
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
  export RE4_R36S_RENDER_SCALE=0.75
  export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
  export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  cd "$CORE" || exit 2
  "./$BIN" "$CORE" "$CORE/modules" "$DATA" "$DURATION"
  rc=$?
  printf 'GEOMETRY_TEST_EXIT_CODE=%s\n' "$rc"
  exit "$rc"
) > "$LOG" 2>&1
rc=$?
printf 'R36S 75%% geometry %s test finished (%s)\nLog: %s\n' "$MODE" "$rc" "$LOG"
sleep 3
exit "$rc"
