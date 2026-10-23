#!/bin/sh
# Cortex-A35 optimized R36S GX game, full internal EFB.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_TEST_RES=full
exec sh "$HERE/RE4-R36S-A35-EFB-Teste.sh"
