#!/usr/bin/env python3
"""Check that RE4NX's freestanding executables are real Linux ARM64 static ELFs."""
from pathlib import Path
import subprocess
import sys

def main():
    if len(sys.argv)<2:
        print('Usage: check_abi.py ELF1 [ELF2 ...]',file=sys.stderr)
        return 2
    for name in sys.argv[1:]:
        p=Path(name)
        try:
            hdr=subprocess.check_output(['readelf','-h',str(p)],text=True,stderr=subprocess.STDOUT)
            prog=subprocess.check_output(['readelf','-l',str(p)],text=True,stderr=subprocess.STDOUT)
            dynamic=subprocess.check_output(['readelf','-d',str(p)],text=True,stderr=subprocess.STDOUT)
        except (OSError,subprocess.CalledProcessError) as e:
            print('ABI_CHECK=FAIL REASON='+str(e),file=sys.stderr)
            return 1
        if ('Machine:                           AArch64' not in hdr or
            'Class:                             ELF64' not in hdr or
            'INTERP' in prog or '(NEEDED)' in dynamic):
            print(f'ABI_CHECK=FAIL BINARY={p.name} machine/static requirements violated',file=sys.stderr)
            return 1
        print(f'ABI_CHECK=PASS BINARY={p.name} MACHINE=AARCH64 LINK=STATIC ABI=LINUX')
    return 0
if __name__=='__main__':raise SystemExit(main())
