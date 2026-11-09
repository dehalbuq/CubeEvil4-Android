#!/bin/sh
# Phase5 synthetic Horizon SVC bridge for Linux AArch64 Cortex-A35.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
: "${CC:=clang}"
"$CC" --target=aarch64-linux-gnu -fuse-ld=lld -nostdlib -static -ffreestanding \
  -mcpu=cortex-a35 -fno-stack-protector -fno-builtin -fno-pic -O2 \
  -Wall -Wextra -Werror -Wpedantic -Wl,-e,_start -Wl,--build-id=none \
  "$HERE/src/native_phase5.c" -o "$HERE/re4nx-svc-aarch64"
file "$HERE/re4nx-svc-aarch64"
