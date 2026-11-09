#!/usr/bin/env python3
import hashlib
from pathlib import Path
import sys
import tempfile
import zipfile
import warnings
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_profile import verify

def expect_fail(p, fragment):
    try:verify(p)
    except ValueError as e:
        assert fragment in str(e), (fragment,str(e))
        return
    raise AssertionError('Invalid archive incorrectly validated: '+fragment)

def main():
    with tempfile.TemporaryDirectory() as tmp:
        p=Path(tmp)/'safe.zip'
        base={
            'profile.env':b'PROFILE_VERSION=1\nARCH=aarch64\nKERNEL_RELEASE=4.4.189\nRAM_TOTAL_MIB=895\nCPU_LOGICAL=4\n',
            'os/uname.txt':b'Linux 4.4.189 aarch64\n',
            'cpu/cpuinfo-redacted.txt':b'processor 0\n',
            'ram/meminfo.txt':b'MemTotal: 916480 kB\n',
            'gpu/gpu-nodes.txt':b'/dev/mali0\n',
        }
        def save(additions={}, manifest_base=None):
            actual={**base, **additions}
            checks=manifest_base if manifest_base is not None else actual
            sums=''.join(f'{hashlib.sha256(v).hexdigest()}  ./{k}\n' for k,v in checks.items())
            actual['SHA256SUMS']=sums.encode()
            with zipfile.ZipFile(p,'w') as z:
                for k,v in actual.items():z.writestr('darkosre-profile/'+k,v)
        save()
        assert verify(p)['hashes_verified']==5
        save({'serial-number':b'secret'})
        expect_fail(p,'sensitive')
        save({'cpu/cpuinfo-redacted.txt':b'tampered'},manifest_base=base)
        expect_fail(p,'Hash mismatch')
        save({'os/extra.txt':b'unlisted'},manifest_base=base)
        expect_fail(p,'unmanifested')
        save()
        with warnings.catch_warnings():
            warnings.simplefilter('ignore',UserWarning)
            with zipfile.ZipFile(p,'a') as z:
                z.writestr('darkosre-profile/os/uname.txt',b'duplicate')
        expect_fail(p,'Duplicate archive member')
        save({'../escape.txt':b'bad'})
        expect_fail(p,'unsafe member')
    print('CLOUD_PROFILE_TESTS=PASS CASES=6')

if __name__=='__main__':main()
