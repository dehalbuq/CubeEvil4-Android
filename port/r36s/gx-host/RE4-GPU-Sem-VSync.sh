#!/bin/sh
# CubeEvil4 GPU isolation - swap mode 1
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_SWAP_MODE=1
exec sh "$HERE/RE4-Primeiro-Boot.sh"
