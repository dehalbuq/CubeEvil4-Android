#!/bin/sh
# R36S / DarkOSRE 64-bit: ORIGINAL CubeEvil4 GX renderer smoke test
# Place this script under /roms/ports and cubeevil4_gx_host/r36s_gx_host beside it.
set -u
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
GAME="$ROOT/cubeevil4_gx_host/r36s_gx_host"
LOG="$ROOT/cubeevil4_gx_host/diagnostico_gx_renderer_darkos.txt"
mkdir -p "$ROOT/cubeevil4_gx_host" || exit 1
(
printf '%s\n' '==== CubeEvil4 original GX renderer / R36S ===='
date
uname -a
printf 'TEST_BINARY=%s\n' "$GAME"
free -m
if [ -f "$GAME" ]; then chmod +x "$GAME" 2>/dev/null || :; fi
if [ ! -x "$GAME" ]; then
  printf '%s\n' 'ERROR=TEST_BINARY_MISSING'
  exit 127
fi
export SDL_VIDEODRIVER=kmsdrm
export SDL_VIDEO_EGL_DRIVER=libEGL.so
# Keep the first GPU test lightweight; base vertex can be enabled later.
export RE4_GX_BASEVERTEX=0
"$GAME" "$@"
status=$?
printf 'EXIT_CODE=%s\n' "$status"
printf 'LOG=%s\n' "$LOG"
exit "$status"
) > "$LOG" 2>&1
result=$?
printf 'GX test exit code: %s\nLog: %s\n' "$result" "$LOG"
# Keep result visible if launched from the frontend.
sleep 3
exit "$result"
