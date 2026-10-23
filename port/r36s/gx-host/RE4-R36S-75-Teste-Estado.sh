#!/bin/sh
# A/B baseline vs safe GX state dedup on R36S Cortex-A35 at FIXED 480x396.
# No changes to the stable launcher, coreproc, save files or game assets.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
MODE="${RE4_R36S_STATE_TEST:-fast}"
case "$MODE" in
  fast) BIN=r36s_game_a35_faststate; LABEL=faststate ;;
  base) BIN=r36s_game_a35_baseline; LABEL=baseline ;;
  *) echo "Invalid RE4_R36S_STATE_TEST (base|fast)";exit 2 ;;
esac
DURATION="${RE4_R36S_SECONDS:-180}"
case "$DURATION" in *[!0-9]*|"") echo "Invalid duration";exit 2;;esac
if [ "$DURATION" -lt 15 ] || [ "$DURATION" -gt 600 ];then
 echo "Duration must be 15..600"; exit 2
fi
CACHE="$CORE/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
LOG="$CORE/diagnostico_a35_75_${LABEL}_darkos.txt"
(
 echo "==== R36S 480x396 GX STATE-DEDUP EXPERIMENT A/B ===="
 date
 uname -a
 free -m
 echo "FRAMEBUFFER=480x396 QUALITY=75%"
 echo "TEST_MODE=$MODE BIN=$BIN"
 echo "SAFETY=NO_DRAW_DISTANCE_CULL NO_OBJECTS_REMOVED"
 echo "QUALITY_LIGHTS=2 DEPTH_OF_FIELD=OFF FOG=OFF"
 echo "GPU_SYNC=MODE_5_GL_FINISH_VSYNC1"
 echo "COMPILER=-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto -ffp-contract=off"
 for entry in "$BIN" re4_coreproc libre4_main.so libre4_platform.so;do
   if [ ! -f "$CORE/$entry" ];then echo "MISSING_FILE=$entry";exit 127;fi
 done
 if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA" ];then
   echo "MISSING_MODULES_OR_GAME_DATA";exit 127
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
 export RE4_R36S_RENDER_SCALE=0.75
 export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
 export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
 cd "$CORE" || exit 2
 "./$BIN" "$CORE" "$CORE/modules" "$DATA" "$DURATION"
 rc=$?
 echo "GX_STATE_TEST_EXIT_CODE=$rc"
 exit "$rc"
) > "$LOG" 2>&1
rc=$?
printf 'RE4 R36S 75%% %s profile ended (%s)\nLog: %s\n' "$MODE" "$rc" "$LOG"
sleep 3
exit "$rc"
