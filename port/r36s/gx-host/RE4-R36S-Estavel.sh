#!/bin/sh
# Resident Evil 4 / R36S DarkOSRE - experimental safe extended test
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_SWAP_MODE=5
export RE4_R36S_SECONDS=180
exec sh "$HERE/RE4-Primeiro-Boot.sh"
