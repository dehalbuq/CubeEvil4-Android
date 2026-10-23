#!/bin/sh
# Test GPU on R36S DarkOSRE. No root, ISO, or package installation.
set -u
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
LOG="$DIR/diagnostico_gles_darkos.txt"
exec > "$LOG" 2>&1
printf '%s\n' '=== CubeEvil4 R36S OpenGL ES diagnostic ==='
date
printf 'REPORT=%s\n' "$LOG"
uname -a
free -m
ls -l /dev/dri/card0 /dev/dri/renderD128 /dev/mali0 2>&1 || :
if ! command -v gcc >/dev/null 2>&1; then
    printf '%s\n' 'ERROR: gcc not available'
    exit 2
fi
if ! gcc -O2 -std=c11 -Wall -Wextra -o "$DIR/r36s_gles_probe.new" "$DIR/r36s_gles_probe.c" -ldl; then
    printf '%s\n' 'ERROR: failed to build diagnostic'
    exit 3
fi
mv "$DIR/r36s_gles_probe.new" "$DIR/r36s_gles_probe"
chmod +x "$DIR/r36s_gles_probe"
"$DIR/r36s_gles_probe" --list || :
printf '%s\n' '--- KMSDRM ---'
SDL_VIDEODRIVER=kmsdrm "$DIR/r36s_gles_probe"
rc=$?
printf 'KMSDRM_EXIT=%s\n' "$rc"
if [ "$rc" -ne 0 ]; then
    printf '%s\n' '--- automatic SDL backend ---'
    (unset SDL_VIDEODRIVER; "$DIR/r36s_gles_probe")
    printf 'AUTO_EXIT=%s\n' "$?"
fi
printf 'REPORT=%s\n' "$LOG"
sleep 2
