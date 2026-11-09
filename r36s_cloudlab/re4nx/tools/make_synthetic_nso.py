#!/usr/bin/env python3
"""Generate a tiny, game-free NSO0 fixture (only code: mov w0,#42; ret).
No commercial data, firmware, keys, or Nintendo proprietary code."""
from pathlib import Path
import hashlib
import struct
import sys


def put32(blob: bytearray, offset: int, val: int) -> None:
    struct.pack_into('<I', blob, offset, val)


def main() -> None:
    dst = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('synthetic-aarch64.nso')
    buf = bytearray(0x100)
    buf[:4] = b'NSO0'
    put32(buf, 0x0c, 0x3f)  # three LZ4-compressed sections with SHA256 verification
    parts = (bytes.fromhex('40058052c0035fd6'), b'R36S', b'TEST')
    offsets = (0x0, 0x1000, 0x2000)
    for i, (content, memory) in enumerate(zip(parts, offsets)):
        off = len(buf)
        block = bytes([len(content) << 4]) + content  # valid raw LZ4 literal block
        buf.extend(block)
        put32(buf, 0x10 + i * 16, off)
        put32(buf, 0x14 + i * 16, memory)
        put32(buf, 0x18 + i * 16, len(content))
        put32(buf, 0x60 + i * 4, len(block))
        buf[0xa0 + i * 32:0xa0 + (i + 1) * 32] = hashlib.sha256(content).digest()
    put32(buf, 0x3c, 0x100)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(buf)
    print(f'SYNTHETIC_NSO_CREATED={dst} SIZE={len(buf)}')


if __name__ == '__main__':
    main()
