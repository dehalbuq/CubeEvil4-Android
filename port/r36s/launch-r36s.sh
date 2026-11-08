#!/bin/sh
# Experimental 0.99.5 IPC GX Linux frontend; no game files distributed.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$HERE"
mkdir -p "$HERE/logs" "$HERE/cache/shader_binaries"
LD_LIBRARY_PATH="$HERE:$(printenv LD_LIBRARY_PATH || true)"
RE4_R36S_LOW_MEM="$(printenv RE4_R36S_LOW_MEM || echo 1)"
# The 0.99.5 core disables only weather and special visual effects at mask 3.
# 0 keeps all effects; 31 is a more aggressive A/B test.
RE4_R36S_POTATO_MASK="$(printenv RE4_R36S_POTATO_MASK || echo 3)"
RE4_GX_BASEVERTEX="$(printenv RE4_GX_BASEVERTEX || echo 0)"
RE4_R36S_SWAP_MODE="$(printenv RE4_R36S_SWAP_MODE || echo 5)"
RE4_R36S_SHADER_CACHE_DIR="$HERE/cache/shader_binaries"
SDL_VIDEO_EGL_DRIVER="$(printenv SDL_VIDEO_EGL_DRIVER || echo libEGL.so)"
SDL_VIDEODRIVER="$(printenv SDL_VIDEODRIVER || echo kmsdrm)"
export LD_LIBRARY_PATH RE4_R36S_LOW_MEM RE4_R36S_POTATO_MASK RE4_GX_BASEVERTEX
export RE4_R36S_SWAP_MODE RE4_R36S_SHADER_CACHE_DIR
export SDL_VIDEO_EGL_DRIVER SDL_VIDEODRIVER
LOG="$HERE/logs/r36s-upstream0995-last.log"
if [ "x${1-}" = "x--probe" ];then
    shift
    exec "$HERE/re4_r36s" --probe "$@" >"$LOG" 2>&1
fi
if [ ! -x "$HERE/r36s_game_showcase_evdev_exit" ] || [ ! -x "$HERE/re4_coreproc" ];then
    echo "R36S missing GX frontend or re4_coreproc; run --probe first" >"$LOG"
    exit 127
fi
for f in libre4_main.so libre4_platform.so libre4_host.so; do
    if [ ! -r "$HERE/$f" ];then echo "MISSING_CORE=$f" >"$LOG";exit 127;fi
done
if [ ! -d "$HERE/modules" ] || [ ! -d "$HERE/data/disc0" ];then
    echo "Missing modules/ or original extracted data/disc0/" >"$LOG"
    exit 127
fi
echo "[R36S] experimental native IPC GX 0.99.5; log=$LOG"
echo "[R36S] exit: hold Start 8s or Start+Select 2s; SIGUSR1/SIGTERM over SSH"
exec "$HERE/r36s_game_showcase_evdev_exit" "$HERE" "$HERE/modules" "$HERE/data" \
    "$(printenv RE4_SHOWCASE_SECONDS || echo 7200)" >"$LOG" 2>&1
