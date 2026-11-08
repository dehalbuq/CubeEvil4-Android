#!/bin/sh
# Experimental launcher. Does not contain original game assets.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$HERE"
LD_LIBRARY_PATH="$HERE:$(printenv LD_LIBRARY_PATH || true)"
RE4_R36S_LOW_MEM="$(printenv RE4_R36S_LOW_MEM || echo 1)"
export LD_LIBRARY_PATH RE4_R36S_LOW_MEM
mkdir -p "$HERE/logs"
echo "[R36S] lowmem=$RE4_R36S_LOW_MEM; log: logs/r36s-last.log"
echo "[R36S] hold Start+Select to exit, or use SIGTERM"
exec "$HERE/re4_r36s" "$@" > "$HERE/logs/r36s-last.log" 2>&1
