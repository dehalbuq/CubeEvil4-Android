#!/bin/sh
# CubeEvil4 R36S DarkOSRE 4:3 gameplay showcase (experimental)
# Uses the user's installed game data and original coreproc; never bundles them.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
for file in r36s_game_showcase re4_coreproc libre4_main.so libre4_platform.so; do
  if [ ! -f "$CORE/$file" ]; then
    echo "CubeEvil4 demo: arquivo ausente: $CORE/$file"
    echo "Esta demonstracao exige uma instalacao do jogo e core compatível."
    sleep 6
    exit 127
  fi
done
if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA/disc0" ]; then
  echo "CubeEvil4 demo: modules/ ou data/disc0/ ausentes."
  echo "Use somente a sua propria copia legal do jogo GameCube G4BE08."
  sleep 6
  exit 127
fi
CACHE="$CORE/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
# zoom: preserve original pixel shape, remove bars but crop side HUD
# stretch: preserve all side content, remove bars, vertical distortion
# original: untouched GameCube letterbox; accessible in SELECT menu by Y
ASPECT="${RE4_SHOWCASE_ASPECT:-zoom}"
case "$ASPECT" in zoom|stretch|original) : ;; *)
  echo "Invalid RE4_SHOWCASE_ASPECT: $ASPECT"
  exit 2
  ;;
esac
export RE4_SHOWCASE_ASPECT="$ASPECT"
export SDL_VIDEODRIVER=kmsdrm
export SDL_VIDEO_EGL_DRIVER=libEGL.so
# Do not override SDL_AUDIODRIVER: DarkOSRE's configured output takes priority.
export RE4_R36S_RENDER_SCALE=0.75
export RE4_R36S_SWAP_MODE=5
export RE4_R36S_SKIP_DOF=1
export RE4_R36S_SKIP_FOG=1
export RE4_R36S_LIGHTS_MAX=2
export RE4_R36S_BASEVERTEX=0
export RE4_GX_BASEVERTEX=0
export RE4_R36S_FAR_Z=0
unset RE4_R36S_UNSAFE_FAR_CULL
export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# This mode is NOT a 3-minute benchmark; it allows 2 hours unless user exits.
DURATION="${RE4_SHOWCASE_SECONDS:-7200}"
case "$DURATION" in *[!0-9]*|"") echo "DURATION_INVALID";exit 2;;esac
if [ "$DURATION" -lt 60 ] || [ "$DURATION" -gt 7200 ];then
  echo "RE4_SHOWCASE_SECONDS range 60..7200";exit 2
fi
chmod +x "$CORE/r36s_game_showcase" "$CORE/re4_coreproc" 2>/dev/null || :
cd "$CORE" || exit 2
# Diagnostic log, console spam and fault-address dumps are disabled in
# the showroom by default. For local debugging only:
# RE4_SHOWCASE_VERBOSE=1 sh RE4-R36S-Demonstracao-43.sh
if [ "${RE4_SHOWCASE_VERBOSE:-0}" = "1" ];then
  exec ./r36s_game_showcase "$CORE" "$CORE/modules" "$DATA" "$DURATION"
fi
./r36s_game_showcase "$CORE" "$CORE/modules" "$DATA" "$DURATION" >/dev/null 2>&1
exit $?
