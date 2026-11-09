#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
: "${CC:=clang}"
"$CC" --target=aarch64-linux-gnu -fuse-ld=lld -nostdlib -static -ffreestanding \
  -fno-stack-protector -fno-builtin -fno-pic -Oz -Wl,-e,_start \
  -Wl,--build-id=none "$HERE/src/bootstrap_aarch64.c" \
  -o "$HERE/bootstrap-aarch64"
file "$HERE/bootstrap-aarch64"
