#!/usr/bin/env python3
"""Negative and positive host tests for the native no-libc NSO validator."""
from pathlib import Path
import hashlib
import subprocess
import struct
import sys
import tempfile
root = Path(__file__).resolve().parent.parent
exe = Path(sys.argv[1]) if len(sys.argv)>1 else root/'build/re4nx-native-host'
fixture = (root/'samples/synthetic-aarch64.nso').read_bytes()

def attempt(data: bytes, mode='--inspect', success=True, marker=''):
    with tempfile.TemporaryDirectory() as tmp:
        file = Path(tmp)/'t.nso'
        file.write_bytes(data)
        p = subprocess.run([str(exe), mode, str(file)], capture_output=True, text=True)
        assert (p.returncode == 0) is success, f'{mode} rc={p.returncode}, output={p.stdout} {p.stderr}'
        assert marker in p.stdout, (marker,p.stdout)

attempt(fixture, '--synthetic', True, 'PHASE2_RESULT=PASS')
attempt(fixture, '--inspect', True, 'NSO_INSPECT=PASS')
attempt(fixture[:140], '--inspect', False, 'FILE_SIZE_INVALID')
wrong_magic=bytearray(fixture);wrong_magic[0:4]=b'NOPE'
attempt(wrong_magic, '--inspect', False, 'NOT_UNENCRYPTED_NSO0')
wrong_sha=bytearray(fixture);wrong_sha[0xa0]^=1
attempt(wrong_sha, '--inspect', False, 'SHA256_MISMATCH')
bad_lz4=bytearray(fixture);bad_lz4[0x100]=0xff
attempt(bad_lz4, '--inspect', False, 'LZ4_DECODE_FAILED')
overlap=bytearray(fixture);struct.pack_into('<I', overlap,0x24,0)
attempt(overlap, '--inspect', False, 'SECTION_OVERLAP')
# Recompute hash after mutating a valid code fixture; the allowlist must deny execution.
not_approved=bytearray(fixture);not_approved[0x101]=0x41
hash0=hashlib.sha256(bytes(not_approved[0x101:0x109])).digest()
not_approved[0xa0:0xc0]=hash0
attempt(not_approved, '--inspect', True, 'PHASE2_RESULT=PASS')
attempt(not_approved, '--synthetic', False, 'NOT_APPROVED_SYNTHETIC_FIXTURE')
print('PHASE2_HOST_NEGATIVE_REGRESSION=PASS TESTS=9')
