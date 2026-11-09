#!/bin/sh
# Entry in the R36S Ports menu; no privilege escalation or package installation.
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
exec sh "$HERE/r36s_cloudlab/collector/coletar-darkosre.sh" "$@"
