#!/bin/sh
# CubeEvil4 first real game data boot; non-destructive, no root needed.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
LOG="$CORE/diagnostico_primeiro_boot_darkos.txt"
MODE="${RE4_R36S_SWAP_MODE:-5}"
case "$MODE" in 0|1|2|3|4|5) ;; *) echo "Invalid RE4_R36S_SWAP_MODE: $MODE"; exit 2;; esac
export RE4_R36S_SWAP_MODE="$MODE"
SECONDS="${RE4_R36S_SECONDS:-180}"
case "$SECONDS" in *[!0-9]*|"") echo "RE4_R36S_SECONDS must be a number 5..600"; exit 2;; esac
if [ "$SECONDS" -lt 5 ] || [ "$SECONDS" -gt 600 ]; then echo "RE4_R36S_SECONDS must be 5..600"; exit 2; fi
mkdir -p "$CORE" || exit 1
(
echo "==== CubeEvil4 / R36S experimental first game-data boot ===="
date
uname -a
free -m
echo "DATA=$DATA"
echo "SWAP_DIAG_MODE=$MODE"
echo "SESSION_SECONDS=$SECONDS"
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
echo "RUNNING_REAL_GAME_BOOT_FOR_MAX_${SECONDS}_SECONDS"
echo "EXIT_COMBO=HOLD_SELECT_AND_START_FOR_2_SECONDS"
./r36s_game_boot "$CORE" "$CORE/modules" "$DATA" "$SECONDS"
status=$?
echo "TEST_EXIT_CODE=$status"
exit "$status"
) > "$LOG" 2>&1
status=$?
MODE_LOG="$CORE/diagnostico_primeiro_boot_modo${MODE}_darkos.txt"
cp "$LOG" "$MODE_LOG" 2>/dev/null || :
printf "Primeiro boot RE4: codigo %s\nRelatorio: %s\nCopia especifica: %s\n" "$status" "$LOG" "$MODE_LOG"
sleep 3
exit "$status"
