#!/bin/sh
# Fixed 480x396 baseline vs A35-O3 CPU game core comparison.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 2
export RE4_CORE_VARIANT=baseline
exec sh "$HERE/RE4-R36S-Comparar-Nucleo.sh"
