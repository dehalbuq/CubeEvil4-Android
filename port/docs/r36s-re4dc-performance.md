# CubeEvil4 on R36S: RE4DC techniques and safe performance baseline

This is a **preflight/measurement change**, not a new game build or a claim of improved FPS.

## Architectural observations (2026-11-02)

- RE4DC (https://github.com/lamb2k/re4dc, dreamcast-port) uses native
  Dreamcast/PowerVR rendering, converted PS2 visuals, shorter visibility,
  reduced foliage and alpha effects. Its Dreamcast binary and texture
  formats cannot be dropped into an RK3326 Linux build.
- The existing Android frontend source
  port/android-decomp/app/src/main/cpp/gx_gl.h targets **OpenGL ES 3.0**.
  It is not an OpenGL ES 2.0 implementation. Actual DarkOSRE/ArkOS EGL
  capabilities must be verified on the console.
- The existing .github/workflows/r36s-arm64-core.yml builds and tests the
  AArch64 game core, but its generated artifact explicitly **does not
  contain native Linux graphics, audio or controller frontends**.
  QEMU tests do not prove a playable R36S build.
- port/ is a partial experimental native foundation. port/whole/ and
  port/android-decomp/ hold the more advanced game core/Android frontend.
  Do not confuse those tracks when reading status reports.

## A/B plan after device data

1. Establish native process memory, CPU usage, firmware, driver and FPS
   measurements in the same scene. Use port/tools/r36s_perf_probe.sh.
2. Verify actual EGL/OpenGL ES version and hardware renderer, not merely the
   presence of libEGL/libGLESv2 files. Identify whether the gameplay frontend
   is actually using GLES 2 or GLES 3.
3. With the same input/camera/scene, benchmark 640x480 / 480x360 / 320x240,
   MSAA off, AF off and postprocessing off; never confuse a frame cap with
   faster emulation. Maintain 4:3 pixel aspect.
4. Profile the exact GX bottleneck: CPU decode, shader compilation/cache,
   draw calls, uniform uploads, EFB copy, GPU stalls, textures and RAM peaks.
5. Only then isolate and test rendering-only RE4DC-style toggles:
   shorter visual distance, simplified far LOD, lower foliage density,
   prelit static scenery, less alpha blending and secondary particles.
   Keep AI, collision, event logic, input, videos and save behavior intact.
6. Compare OFF/ON with real device measurements and gameplay regression
   checks. Do not enable speculative global geometry/effect removal.

## Device measurement without requiring working exit buttons

In SSH, **before** starting the current playable experimental port:

    nohup sh /roms/ports/r36s_perf_probe.sh 180 2 >/dev/null 2>&1 &

Then start and play the game. The probe exits by itself, independently of
in-game hotkeys. Collect the .log and .csv from:

    /roms/ports/cubeevil4-diagnostics/

For a different process name, set RE4_R36S_PROCESS_NAMES='name1 name2'.
For a different log destination, set RE4_R36S_LOG_DIR='/path'.
Use the script locally on each firmware (DarkOSRE, ArkOS, ArkOS4Clone).
No root privileges, performance-mode changes or game data are required.
If the process is not detected, the probe still captures system telemetry.

**This branch does not change gameplay or compile an ARM64 release.**
No game/disc assets are part of this change.
