#!/bin/sh
# Cortex-A35 PS2-safe profile 480x396: test reuse GX vertex/index upload.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_GEOMETRY_MODE=reuse
exec sh "$HERE/RE4-R36S-75-Upload-GX.sh"
