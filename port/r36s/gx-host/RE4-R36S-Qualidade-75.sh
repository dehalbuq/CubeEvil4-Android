#!/bin/sh
# R36S Mali-G31 internal EFB scale 0.75; original game data unchanged.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_RENDER_SCALE="0.75"
exec sh "$HERE/RE4-R36S-Escolher-Resolucao.sh"
