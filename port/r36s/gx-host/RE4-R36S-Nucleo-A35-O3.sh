#!/bin/sh
# CubeEvil4 Resident Evil 4 GC on R36S DarkOSRE
# EXPERIMENT: full original game-core and 114 REL modules rebuilt for A35 O3,
# with GX display-list copy acceleration. The stable core is NEVER replaced.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 2
ROOT="$HERE/cubeevil4_live_gx"
FAST="$ROOT/core_a35_o3"
DATA="$ROOT/data"
FRONT="$ROOT/r36s_game_showcase_a35_o3"
for file in "$FRONT" "$FAST/re4_coreproc" "$FAST/libre4_main.so" "$FAST/libre4_platform.so" "$FAST/libre4_host.so"; do
  if [ ! -s "$file" ];then
    echo "R36S A35 O3: falta o arquivo $file"
    sleep 5
    exit 127
  fi
done
if [ ! -d "$FAST/modules" ] || [ ! -d "$DATA/disc0" ];then
  echo "R36S A35 O3: faltam modules ou data/disc0 (extraia seus proprios dados G4BE08)."
  sleep 5
  exit 127
fi
DURATION="${RE4_SHOWCASE_SECONDS:-7200}"
case "$DURATION" in *[!0-9]*|"")echo "Duracao invalida";exit 2;;esac
if [ "$DURATION" -lt 60 ] || [ "$DURATION" -gt 7200 ];then
  echo "Duracao permitida 60..7200 segundos";exit 2
fi
CACHE="$ROOT/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
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
export LD_LIBRARY_PATH="$FAST:$ROOT${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
chmod +x "$FRONT" "$FAST/re4_coreproc" 2>/dev/null || :
cd "$ROOT" || exit 2
# Original GameCube game data and save card path are passed UNMODIFIED;
# only the executable + rebuilt game shared libs come from core_a35_o3.
if [ "${RE4_SHOWCASE_INPUT_DIAG:-0}" = "1" ];then
  mkdir -p "$ROOT/logs" || exit 2
  "$FRONT" "$FAST" "$FAST/modules" "$DATA" "$DURATION" \
    >"$ROOT/logs/diagnostico_controles_a35_o3.txt" 2>&1
  exit $?
fi
if [ "${RE4_SHOWCASE_VERBOSE:-0}" = "1" ];then
  exec "$FRONT" "$FAST" "$FAST/modules" "$DATA" "$DURATION"
fi
"$FRONT" "$FAST" "$FAST/modules" "$DATA" "$DURATION" >/dev/null 2>&1
exit $?
