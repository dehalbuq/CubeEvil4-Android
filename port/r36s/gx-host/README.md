# CubeEvil4 original GX renderer on R36S (DarkOSRE Linux ARM64)

This milestone uses the **real `GxRenderer` code** from the CubeEvil4 Android core mode. It is not a replacement renderer or a full playable Linux port.

## What it tests

The existing `gx_gl.cpp`, `gx_gl_shader.cpp` and `gx_texture.cpp` are compiled natively for Linux AArch64 and linked with SDL2 / OpenGL ES.

On the R36S:
1. SDL2 creates a KMSDRM GLES3 context.
2. The original renderer creates its GameCube-sized EFB buffers, GL objects and utility shaders.
3. A synthetic original GX command stream clears blue and draws a green quad and a red triangle with the real TEV shader translator.
4. It verifies three pixel colors in the GPU-produced XFB and presents the result on the SDL2 window.
5. Graphics errors, shader statistics, pixel colors, and memory consumption are logged.

**No ISO is required.** The test should display a blue background with green and red shapes; the log independently checks GPU pixel colors. The authoritative result is `GX_REAL_SHADERS_TRIANGLES_AND_XFB_OK`.

A single file argument can be used to replay an authentic `RE4GXCH1` command chunk dumped by CubeEvil4 tooling, but do not share copyrighted game assets publicly. It caps each input at 48 MiB; sequential frame playback is not yet implemented.

## Installation

1. Open the **Actions** tab, choose `R36S DarkOSRE - native CubeEvil4 GX renderer` and download the `CubeEvil4-R36S-GX-Renderer-ARM64` artifact after a successful build.
2. Extract the ZIP into `/roms/ports`, keeping the folder structure:
   - `/roms/ports/RE4-GX-Renderer-Test.sh`
   - `/roms/ports/cubeevil4_gx_host/r36s_gx_host`
3. Launch `RE4-GX-Renderer-Test.sh` from Ports, or run `sh /roms/ports/RE4-GX-Renderer-Test.sh` if DRM master is available.

The report appears at:

`/roms/ports/cubeevil4_gx_host/diagnostico_gx_renderer_darkos.txt`

## Known limitations

- Linux ARM64 graphics initialization test only, no game boot, input mapping, audio, menu or game loop.
- Memory and real shader behavior must be measured on the Mali-G31 itself.
- The GPU may reject shaders/FBO configurations accepted by x86 Mesa.
- The synthetic GX frame exercises polygon drawing with the game's GX renderer, but does not contain game characters or room geometry.
- No claim of playable Resident Evil 4 or acceptable FPS until actual gameplay measurements.

## Source

- `port/r36s/gx-host/r36s_gx_host.cpp`: platform SDL2 host and synthetic GX packet
- `port/android-decomp/app/src/main/cpp/gx_gl.cpp`: existing actual GX renderer
- `port/whole/runtime/re4_gx_frame.h`: GX packet interface
