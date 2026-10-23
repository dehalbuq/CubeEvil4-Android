#!/bin/sh
# R36S Cortex-A35 480x396 GX comparison mode base
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_STATE_TEST=base
exec sh "$HERE/RE4-R36S-75-Teste-Estado.sh"
