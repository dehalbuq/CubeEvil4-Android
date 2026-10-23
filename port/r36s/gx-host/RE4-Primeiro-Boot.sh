#!/bin/sh
# CubeEvil4 first real game data boot; non-destructive, no root needed.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
LOG="$CORE/diagnostico_primeiro_boot_darkos.txt"
mkdir -p "$CORE" || exit 1
(
echo "==== CubeEvil4 / R36S experimental first game-data boot ===="
date
uname -a
free -m
echo "DATA=$DATA"
if [ ! -f "$CORE/r36s_game_boot" ]; then
  echo "ERROR=BOOT_EXECUTABLE_MISSING"
  exit 127
fi
chmod +x "$CORE/r36s_game_boot" 2>/dev/null || :
if [ ! -x "$CORE/re4_coreproc" ]; then chmod +x "$CORE/re4_coreproc" 2>/dev/null || :; fi
if [ ! -f "$CORE/libre4_main.so" ] || [ ! -f "$CORE/libre4_platform.so" ] || [ ! -d "$CORE/modules" ]; then
  echo "ERROR=CORE_LIBRARIES_MISSING"
  exit 127
fi
if [ ! -d "$DATA" ]; then
  echo "ERROR=EXTRACTED_GAME_DATA_NOT_FOUND"
  echo "Expected /roms/ports/cubeevil4_live_gx/data/disc0/{sys,files} or disc1/{sys,files}"
  exit 21
fi
export SDL_VIDEODRIVER=kmsdrm
export SDL_VIDEO_EGL_DRIVER=libEGL.so
export RE4_GX_BASEVERTEX=0
export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$CORE" || exit 2
echo "RUNNING_REAL_GAME_BOOT_FOR_MAX_45_SECONDS"
./r36s_game_boot "$CORE" "$CORE/modules" "$DATA" 45
status=$?
echo "TEST_EXIT_CODE=$status"
exit "$status"
) > "$LOG" 2>&1
status=$?
printf "Primeiro boot RE4: codigo %s\nRelatorio: %s\n" "$status" "$LOG"
sleep 3
exit "$status"
