#!/usr/bin/env python3
"""Host-only negative and positive tests; does not execute guest Switch code."""
from pathlib import Path
import subprocess
import sys
exe = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path('/tmp/re4nx-hle-host')

def run(args, rc, *markers):
    p = subprocess.run([str(exe), *args], capture_output=True, text=True, check=False, timeout=15)
    assert p.returncode == rc, (p.returncode, p.stdout, p.stderr)
    for marker in markers:
        assert marker in p.stdout, (marker, p.stdout)
    return p.stdout

run(['--selftest'], 0,
    'HLE_IPC_SYNTHETIC=PASS',
    'MAX=64 OVERFLOW=REJECTED',
    'STALE_HANDLE=REJECTED',
    'HEAP_DENIED_MIB=705',
    'RWX_DENIED=YES',
    'RAM_COMMIT_READBACK=PASS',
    'GUEST_GAME_EXECUTION=DISABLED',
    'PHASE3_FINAL=PASS')
run(['--stress'], 0, 'HANDLE_STRESS=PASS CYCLES=10000 LIVE_HANDLES=0', 'PHASE3_STRESS_FINAL=PASS')
run(['--not-a-mode'], 1, 'PHASE3_FINAL=FAIL REASON=UNKNOWN_OPTION')
print('PHASE3_HOST_TESTS=PASS CASES=3')
