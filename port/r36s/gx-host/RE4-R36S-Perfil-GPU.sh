#!/bin/sh
# CubeEvil4 R36S: performance profiling of real GX frame stages.
# This NEVER overwrites the stable launcher binary or its last report.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
LOG="$CORE/diagnostico_perfil_gpu_darkos.txt"
SECONDS="${RE4_R36S_PROFILE_SECONDS:-180}"
case "$SECONDS" in *[!0-9]*|"") echo "Invalid profiling duration" ; exit 2;; esac
if [ "$SECONDS" -lt 15 ] || [ "$SECONDS" -gt 600 ]; then
  echo "RE4_R36S_PROFILE_SECONDS must be 15..600"; exit 2
fi
mkdir -p "$CORE" || exit 1
(
echo "==== R36S GPU FRAME PROFILER ===="
date
uname -a
free -m
echo "PROFILE_DURATION_SECONDS=$SECONDS"
echo "PROFILE_MODE=5_GL_FINISH_VSYNC1"
for entry in r36s_game_profile re4_coreproc libre4_main.so libre4_host.so libre4_platform.so; do
  if [ ! -f "$CORE/$entry" ]; then echo "MISSING_FILE=$entry"; exit 127; fi
done
if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA" ]; then
  echo "MISSING_GAME_DATA_OR_MODULES=1"
  exit 127
fi
chmod +x "$CORE/r36s_game_profile" "$CORE/re4_coreproc" 2>/dev/null || :
export SDL_VIDEODRIVER=kmsdrm
export SDL_VIDEO_EGL_DRIVER=libEGL.so
export RE4_GX_BASEVERTEX=0
export RE4_R36S_SWAP_MODE=5
export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$CORE" || exit 2
./r36s_game_profile "$CORE" "$CORE/modules" "$DATA" "$SECONDS"
rc=$?
printf "PROFILER_EXIT_CODE=%s\n" "$rc"
exit "$rc"
) > "$LOG" 2>&1
rc=$?
printf "Perfil GPU finalizado (codigo %s)\nRelatorio: %s\n" "$rc" "$LOG"
sleep 3
exit "$rc"
