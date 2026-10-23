# CubeEvil4: R36S core -> graphics over shared-memory IPC

Experimental **live** bridge: a native SDL2/KMSDRM GLES3 frontend starts a separate
AArch64 `re4_coreproc` process. It reads that process's decoded GameCube GX command
chunks with the EXISTING `re4ipc_gx_acquire` shared-memory protocol and draws them
with the project's original `re4gx::GxRenderer`.

**No ISO or game data is needed for this milestone**. By design, the test sets
`RE4_CORE_TEST_ENTRY=remote`, which runs the upstream core's synthetic triangle
producer. This is a hardware integration test, NOT playable Resident Evil 4.

## Why a separate process?

The game's original 32-bit addresses, represented in the AArch64
`0x83000000..0xFFFFFFFF` mapping window, must not collide with mappings
from SDL2, Mali EGL or the Linux frontend. The IPC mode the Android app already
uses is much safer to port. For AArch64 Linux, `re4_coreproc` is now built and
reserves high virtual memory before loading game libraries, using the same
address-window strategy as the existing `re4_host`.

## How to build

GitHub Actions `R36S - live original core to GX (AArch64)` builds
the AArch64 core with official cross-compilation scripts, builds the
SDL2 frontend natively on Ubuntu ARM64, and assembles a single installable
artifact called `CubeEvil4-R36S-Live-Core-GX`.

The release zip should contain:

    RE4-Core-GX-Ao-Vivo.sh
    cubeevil4_live_gx/
      r36s_live_ipc
      re4_coreproc
      libre4_main.so
      libre4_host.so
      libre4_platform.so
      modules/libre4_*.so

Extract into `/roms/ports`, keeping the folder structure.
From **Ports**, run `RE4-Core-GX-Ao-Vivo.sh`.
It tries receiving and presenting GX frames for up to four seconds.
The output report is:

    /roms/ports/cubeevil4_live_gx/diagnostico_core_gx_ao_vivo_darkos.txt

## What counts as success?

- `RESULT=LIVE_CORE_IPC_GX_RENDER_OK` and exit code 0
- At least three genuine core-generated GX packets, at least two presented frames
- No IPC checksum mismatches or shader/GL failures, and an initial RESET packet

If shader errors appear, the code still logs the exact issue for targeted adaptation.
If a new build does not compile, inspect the workflow logs. The real
Mali-G31 driver can behave differently from the CI's software GPU.

**Remaining work after this test:** game data extraction, proper game entry
instead of the synthetic probe, input mapping, audio output, saves,
performance benchmarks, and memory management under ~895 MiB RAM.
