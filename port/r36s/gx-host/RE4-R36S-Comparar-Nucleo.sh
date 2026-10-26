#!/bin/sh
# Apples-to-apples 180s GX/CPU-core experiment. Identical diagnostic frontend
# and 480x396 GLES settings, different AArch64 core + 114 module libraries.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 2
ROOT="$HERE/cubeevil4_live_gx"
MODE="${RE4_CORE_VARIANT:-a35}"
case "$MODE" in
  a35)CORE="$ROOT/core_a35_o3";;
  baseline)CORE="$ROOT";;
  *)echo "Modo invalido: $MODE (a35|baseline)";exit 2;;
esac
BIN="$ROOT/r36s_game_a35_core_compare"
DATA="$ROOT/data"
DURATION="${RE4_CORE_TEST_SECONDS:-180}"
case "$DURATION" in *[!0-9]*|"")echo "Duracao invalida";exit 2;;esac
if [ "$DURATION" -lt 30 ] || [ "$DURATION" -gt 600 ];then
  echo "Duracao permitida 30..600 segundos";exit 2
fi
for f in "$BIN" "$CORE/re4_coreproc" "$CORE/libre4_main.so" "$CORE/libre4_platform.so"; do
  if [ ! -s "$f" ];then echo "FALTANDO=$f";exit 127;fi
done
if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA/disc0" ];then
  echo "FALTANDO=modules ou data/disc0";exit 127
fi
CACHE="$ROOT/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
LOG="$ROOT/diagnostico_core_${MODE}_480x396_darkos.txt"
(
  echo "=== RESIDENT EVIL 4 R36S CORE O2 versus O3/A35 GX FIFO ==="
  date
  uname -a
  echo "MODE=$MODE FRONTEND=r36s_game_a35_core_compare"
  echo "CORE_DIR=$CORE"
  echo "EFB=480x396 VISUAL=2lights_DOFoff_FOGoff_no_far_cull"
  echo "MALI_SWAP=5 GLFINISH_BEFORE_SWAP=1"
  if [ "$MODE" = "a35" ];then
    echo "CORE_COMPILER=O3_-mcpu=cortex-a35_-mtune=cortex-a35_FAST_FIFO=1"
  else
    echo "CORE_COMPILER=known_working_O2"
  fi
  free -m
  export SDL_VIDEODRIVER=kmsdrm
  export SDL_VIDEO_EGL_DRIVER=libEGL.so
  export RE4_R36S_RENDER_SCALE=0.75
  export RE4_R36S_SWAP_MODE=5
  export RE4_R36S_SKIP_DOF=1
  export RE4_R36S_SKIP_FOG=1
  export RE4_R36S_LIGHTS_MAX=2
  export RE4_R36S_FAR_Z=0
  export RE4_GX_BASEVERTEX=0
  unset RE4_R36S_UNSAFE_FAR_CULL
  export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
  export LD_LIBRARY_PATH="$CORE:$ROOT${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  chmod +x "$BIN" "$CORE/re4_coreproc" 2>/dev/null || :
  cd "$ROOT" || exit 2
  "$BIN" "$CORE" "$CORE/modules" "$DATA" "$DURATION"
  rc=$?
  echo "CORE_VARIANT_EXIT_CODE=$rc"
  exit "$rc"
) >"$LOG" 2>&1
rc=$?
printf 'R36S Core %s test terminou (%s)\nRelatorio: %s\n' "$MODE" "$rc" "$LOG"
sleep 3
exit "$rc"
