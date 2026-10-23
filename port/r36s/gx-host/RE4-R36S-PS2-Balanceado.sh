#!/bin/sh
# Resident Evil 4 R36S opt-in PS2-inspired graphics preset 1.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_PS2_PRESET=1
exec sh "$HERE/RE4-R36S-PS2-LowSpec.sh"
