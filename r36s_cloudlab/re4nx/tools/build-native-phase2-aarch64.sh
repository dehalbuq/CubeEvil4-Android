#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
: "${CC:=clang}"
"$CC" --target=aarch64-linux-gnu -fuse-ld=lld -nostdlib -static -ffreestanding \
  -mcpu=cortex-a35 -fno-stack-protector -fno-builtin -fno-pic -O2 -Wall -Wextra -Werror \
  -Wl,-e,_start -Wl,--build-id=none \
  "$HERE/src/native_phase2.c" -o "$HERE/re4nx-native-aarch64"
file "$HERE/re4nx-native-aarch64"
