#!/bin/sh
# Unofficial Resident Evil 4 R36S demonstration. Own original game data required.
# Output is the native 640x480 panel. 1x EFB; never upscale a reduced EFB.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$HERE"
if [ ! -x "$HERE/r36s_re4_demo_640x480" ] || [ ! -x "$HERE/re4_coreproc" ]; then
  echo "Resident Evil 4 R36S Demo: executaveis ARM64 ausentes."
  exit 127
fi
for f in libre4_main.so libre4_platform.so libre4_host.so; do
  if [ ! -r "$HERE/$f" ]; then
    echo "Resident Evil 4 R36S Demo: nucleo compativel ausente ($f)."
    exit 127
  fi
done
if [ ! -d "$HERE/modules" ] || [ ! -d "$HERE/data/disc0" ]; then
  echo "Resident Evil 4 R36S Demo: forneca seus proprios arquivos originais em data/disc0/."
  exit 127
fi
mkdir -p "$HERE/cache/shader_binaries"
export LD_LIBRARY_PATH="$HERE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-kmsdrm}"
export SDL_VIDEO_EGL_DRIVER="${SDL_VIDEO_EGL_DRIVER:-libEGL.so}"
# Do NOT force audio driver, resolution downscaling, debug trace or OpenGL extensions.
export RE4_R36S_LOW_MEM=1
export RE4_R36S_POTATO_MASK="${RE4_R36S_POTATO_MASK:-31}"
export RE4_R36S_SWAP_MODE=5
export RE4_GX_BASEVERTEX=0
export RE4_GX_TV_FILTER=0
export RE4_GX_PSX=0
export RE4_GX_FLARES=0
export RE4_SHOWCASE_INPUT_DIAG=0
export RE4_R36S_SHADER_CACHE_DIR="$HERE/cache/shader_binaries"
# SDL creates a 640x480 GLES3 window; release frontend uses nearest XFB sampling.
# No console spam, FPS HUD, profiler dumps, debug log or raw crash trace.
exec "$HERE/r36s_re4_demo_640x480" "$HERE" "$HERE/modules" "$HERE/data" \
  "${RE4_SHOWCASE_SECONDS:-7200}" >/dev/null 2>&1
