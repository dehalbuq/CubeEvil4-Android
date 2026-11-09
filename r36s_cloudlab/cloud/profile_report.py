#!/usr/bin/env python3
"""Generate a privacy-minimized, reproducible dArkOSRE target profile.

Reads the verified collector archive in memory; NEVER extracts/executes members.
The JSON is safe to commit only after user review; we do not include paths,
serials, hostname, addresses, peripheral IDs, or raw library inventories.
"""
import argparse
import datetime
import json
from pathlib import Path
import re
from verify_profile import verify, load


def value(files, path):
    return files.get('darkosre-profile/' + path, b'').decode('utf-8', 'replace').strip('\x00\n ')


def integer(text, fallback=None):
    match = re.search(r'\d+', text or '')
    return int(match.group()) if match else fallback


def device_profile(archive):
    verify(Path(archive))
    files=load(Path(archive))
    p = {}
    for line in value(files, 'profile.env').splitlines():
        if '=' in line:
            k,v=line.split('=',1)
            if re.fullmatch('[A-Z0-9_]+',k):p[k]=v
    cpu=value(files, 'cpu/lscpu.txt')
    mem=value(files, 'ram/meminfo.txt')
    def colon_label(contents, name):
        m=re.search(r'^'+re.escape(name)+r':\s*(.+)$', contents, re.MULTILINE)
        return m.group(1).strip() if m else None
    page=integer(value(files,'cpu/page-size.txt'))
    mem_total=integer(colon_label(mem,'MemTotal'))
    mem_avail=integer(colon_label(mem,'MemAvailable'))
    gpu=value(files,'gpu/devfreq.txt')
    gpu_block=re.search(r'DEVFREQ_DEVICE=ff400000.gpu\n(.*?)(?=DEVFREQ_DEVICE=|\Z)',gpu,re.S)
    block=gpu_block.group(1) if gpu_block else ''
    def section(name):
        m=re.search(r'^\['+re.escape(name)+r'\]\s*(.*?)(?=\n\[|\Z)',block,re.M|re.S)
        return m.group(1).strip() if m else ''
    drm=value(files,'gpu/drm-output.txt')
    mode=re.search(r'(\d+)x(\d+)p(\d+)',drm)
    model=value(files,'cpu/device-model.txt')
    abi='aarch64' if p.get('ARCH')=='aarch64' else p.get('ARCH','unknown')
    # The collector does not capture GL_VENDOR/GL_RENDERER. A prior user-owned
    # GPU runtime log identified Mali-G31/ES3.2; retain it as separate evidence.
    total_mib=mem_total//1024 if mem_total is not None else integer(p.get('RAM_TOTAL_MIB',''))
    available_mib=mem_avail//1024 if mem_avail is not None else None
    report={
        'schema_version':2,
        'source':'dArkOSRE local collector snapshot; technical metadata only',
        'snapshot_verified':True,
        'model':model,
        'device_compatible':value(files,'cpu/device-compatible.txt').splitlines(),
        'os_pretty_name':colon_label(value(files,'os/os-release.txt'),'PRETTY_NAME') or re.search(r'^PRETTY_NAME=(.*)$',value(files,'os/os-release.txt'),re.M).group(1).strip('"') if 'PRETTY_NAME=' in value(files,'os/os-release.txt') else 'unknown',
        'kernel':p.get('KERNEL_RELEASE'),
        'arch':abi,
        'cpu':{
            'model':colon_label(cpu,'Model name'),
            'cores':integer(p.get('CPU_LOGICAL')),
            'max_khz':integer(value(files,'cpu/cpu0/scaling_max_freq.txt')),
            'current_khz':integer(value(files,'cpu/cpu0/scaling_cur_freq.txt')),
            'governor':value(files,'cpu/cpu0/scaling_governor.txt'),
            'page_size_bytes':page,
        },
        'memory':{
            'reported_total_mib':total_mib,
            'reported_available_mib_at_capture':available_mib,
            'swap_mib':0 if 'SwapTotal:             0 kB' in mem else integer(colon_label(mem,'SwapTotal'), 0)//1024,
            'initial_target_process_virtual_mib':768,
            'recommended_rss_budget_mib':512,
            'optional_rss_stress_mib':640,
            'stress_levels_mib':[64,256,512],
        },
        'gpu':{
            'devfreq_device':'ff400000.gpu' if gpu_block else None,
            'max_hz':integer(section('max_freq')),
            'current_hz':integer(section('cur_freq')),
            'governor':section('governor'),
            'observed_previous_runtime_renderer':'Mali-G31 (older CubeEvil4 hardware log; not verified by this collector)',
            'previous_runtime_gles':'OpenGL ES 3.2 (older CubeEvil4 hardware log; not verified by this collector)',
            'cloud_gpu_is_real_mali':False,
        },
        'display':{'width':int(mode.group(1)) if mode else None,'height':int(mode.group(2)) if mode else None,'nominal_hz':int(mode.group(3)) if mode else None},
        'input':{'gamepad_name':'GO-Super Gamepad' if 'GO-Super Gamepad' in value(files,'input/devices-redacted.txt') else None,'secondary_keypad_name':'rk29-keypad' if 'rk29-keypad' in value(files,'input/devices-redacted.txt') else None},
        'libraries':{'sdl2_config_version':value(files,'toolchain/sdl2-version.txt'),'sdl2_pkg_config_version':value(files,'toolchain/sdl2-pkg-version.txt'),'egl_pkg_config_version':value(files,'toolchain/egl-pkg-version.txt'),'glesv2_pkg_config_version':value(files,'toolchain/glesv2-pkg-version.txt')},
        'warnings':[
            'Emulated host CPU/memory timings and software GLES are not equivalent to Cortex-A35/Mali-G31',
            '768 MiB is a virtual address book-keeping target, not a safe physical RSS allocation',
            'Firmware clock/timestamps may be wrong; snapshot filename predates the recorded kernel build date',
            'Kernel 4.4.189 system call and ioctl behavior cannot be fully reproduced on newer cloud kernels',
        ],
    }
    return report


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('archive',type=Path)
    ap.add_argument('--output',type=Path)
    args=ap.parse_args()
    report=device_profile(args.archive)
    data=json.dumps(report,ensure_ascii=False,indent=2,sort_keys=True)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(data,encoding='utf-8')
        print('TARGET_HARDWARE_PROFILE=PASS OUTPUT='+str(args.output))
    else:print(data)

if __name__=='__main__':main()
