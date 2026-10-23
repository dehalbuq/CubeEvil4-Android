#!/bin/sh
# R36S GPU optimization A/B: exclude ONLY diagnostic GPU timer queries and per-chunk progress fences.
# Synchronization for the uniform stream and glFinish before EGL swap remain enabled.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_GPU_PROBES=0
export RE4_R36S_BASEVERTEX=0
export RE4_R36S_DEEP_SECONDS=180
exec sh "$HERE/RE4-R36S-Perfil-GX-Profundo.sh"
