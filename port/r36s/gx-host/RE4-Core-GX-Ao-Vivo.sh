#!/bin/sh
# R36S / DarkOSRE ARM64: live GX frames from the core process (no ISO).
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
DIR="$HERE/cubeevil4_live_gx"
LOG="$DIR/diagnostico_core_gx_ao_vivo_darkos.txt"
mkdir -p "$DIR" || exit 2
(
printf "%s\n" "==== CubeEvil4 R36S - IPC core->GX original ===="
date
uname -a
free -m
printf "GAME_DIR=%s\n" "$DIR"
if [ ! -x "$DIR/r36s_live_ipc" ] || [ ! -x "$DIR/re4_coreproc" ]; then
  chmod +x "$DIR/r36s_live_ipc" "$DIR/re4_coreproc" 2>/dev/null || :
fi
for file in "$DIR/r36s_live_ipc" "$DIR/re4_coreproc" "$DIR/libre4_main.so" \
            "$DIR/libre4_host.so" "$DIR/libre4_platform.so"; do
  if [ ! -r "$file" ]; then
    printf "MISSING_FILE=%s\n" "$file"
    exit 127
  fi
done
if [ ! -d "$DIR/modules" ]; then
  printf "MISSING_DIRECTORY=%s\n" "$DIR/modules"
  exit 127
fi
export SDL_VIDEODRIVER=kmsdrm
export SDL_VIDEO_EGL_DRIVER=libEGL.so
export RE4_GX_BASEVERTEX=0
export LD_LIBRARY_PATH="$DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$DIR" || exit 2
./r36s_live_ipc "$DIR" "$DIR/modules" 4
code=$?
printf "RESULT_EXIT=%s\n" "$code"
printf "REPORT=%s\n" "$LOG"
exit "$code"
) > "$LOG" 2>&1
rc=$?
printf "Teste Core/GX finalizado (codigo %s)\nRelatorio: %s\n" "$rc" "$LOG"
sleep 3
exit "$rc"
