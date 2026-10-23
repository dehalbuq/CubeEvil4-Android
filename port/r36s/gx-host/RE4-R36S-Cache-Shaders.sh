#!/bin/sh
# R36S DarkOSRE: experimental persistent Mali shader binary cache.
# Keeps the known-good r36s_game_boot and launcher untouched.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
CACHE="$CORE/cache/shader_binaries"
RUNFILE="$CACHE/.run_number"
mkdir -p "$CACHE" || { echo "SHADER_CACHE_DIR_CREATE_FAILED"; exit 2; }
if [ -r "$RUNFILE" ]; then PREVIOUS=$(cat "$RUNFILE" 2>/dev/null); else PREVIOUS=0; fi
case "$PREVIOUS" in *[!0-9]*|"") PREVIOUS=0 ;; esac
if [ "$PREVIOUS" -ge 99 ]; then PREVIOUS=0; fi
RUN=$((PREVIOUS+1))
printf '%s\n' "$RUN" > "$RUNFILE" || exit 2
LOG="$CORE/diagnostico_shader_cache_teste${RUN}_darkos.txt"
SECONDS="${RE4_R36S_SECONDS:-180}"
case "$SECONDS" in *[!0-9]*|"") echo "DURATION_INVALID"; exit 2;; esac
if [ "$SECONDS" -lt 15 ] || [ "$SECONDS" -gt 600 ]; then echo "DURATION_OUT_OF_RANGE=15..600"; exit 2; fi
(
  echo "==== R36S MALIG31 SHADER BINARY CACHE TEST ===="
  date
  uname -a
  free -m
  df -Pk "$CACHE"
  echo "SHADER_CACHE_RUN_NUMBER=$RUN"
  echo "SHADER_CACHE_DURATION_SECONDS=$SECONDS"
  echo "SHADER_CACHE_FILES_BEFORE=$(find "$CACHE" -type f -name '*.bin' | wc -l)"
  du -sk "$CACHE"
  for entry in r36s_game_shadercache re4_coreproc libre4_main.so libre4_platform.so; do
    if [ ! -f "$CORE/$entry" ]; then echo "MISSING_FILE=$entry"; exit 127; fi
  done
  if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA" ]; then
    echo "MISSING_GAME_DATA_OR_MODULES=1"; exit 127
  fi
  chmod +x "$CORE/r36s_game_shadercache" "$CORE/re4_coreproc" 2>/dev/null || :
  export SDL_VIDEODRIVER=kmsdrm
  export SDL_VIDEO_EGL_DRIVER=libEGL.so
  export RE4_GX_BASEVERTEX=0
  export RE4_R36S_SWAP_MODE=5
  export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
  export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  cd "$CORE" || exit 2
  ./r36s_game_shadercache "$CORE" "$CORE/modules" "$DATA" "$SECONDS"
  rc=$?
  echo "SHADER_CACHE_FILES_AFTER=$(find "$CACHE" -type f -name '*.bin' | wc -l)"
  du -sk "$CACHE"
  printf 'SHADER_CACHE_TEST_EXIT_CODE=%s\n' "$rc"
  exit "$rc"
) > "$LOG" 2>&1
rc=$?
printf 'R36S shader cache run %s complete (code %s)\nLog: %s\n' "$RUN" "$rc" "$LOG"
sleep 3
exit "$rc"
