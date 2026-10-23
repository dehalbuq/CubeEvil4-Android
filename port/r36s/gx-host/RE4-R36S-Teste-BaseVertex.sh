#!/bin/sh
# R36S GLES3.2 BaseVertex optimization experiment; stable executable untouched.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
export RE4_R36S_BASEVERTEX=1
export RE4_R36S_PROFILE_SECONDS=180
exec sh "$HERE/RE4-R36S-Perfil-GPU.sh"
