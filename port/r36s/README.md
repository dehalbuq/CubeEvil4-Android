# CubeEvil4 native R36S frontend – experimental Phase 2

This **SDL2 + OpenGL ES 3** frontend is not a finished port. It
directly uses the updated upstream GX rendering backend with Linux
SDL2 and the whole-program core interface. It does not use Dolphin,
Android JNI, or assets from the game repository.

## Implemented experimentally

- Linux SDL2 fullscreen output with 4:3 viewport.
- Explicit GLES 3.0+ initialization, GPU information and core GX setup.
- Optional low-memory policy RE4_R36S_LOW_MEM=1; default remains off
  in the executable; launch-r36s.sh enables it for device testing.
- A native interface to the user-built ARM64 libre4_main.so core:
  reserve/start, decoded GX chunk rendering, gamepad, 32 kHz stereo,
  depth feedback, and a 60 Hz independent VI clock.
- Hold Start+Select for 1.5 seconds for clean frontend exit.
  Escape, Enter+RightShift, SDL quit and SIGTERM are alternatives.
  Default raw buttons are Start=9, Select=8; override them with
  RE4_R36S_START_BUTTON and RE4_R36S_SELECT_BUTTON.
- Logs sent to stderr; launch script logs to logs/r36s-last.log.

This is **source integration**, not a verified running R36S game.
The core libraries, memory mappings, actual rendering, user-supplied
data, controller mappings, audio and performance all still require
hardware testing. A 60 Hz VI clock does not guarantee 60 rendered FPS.

## Build on Linux

Install SDL2 development, EGL and GLESv2/GLES3 headers and library:

    cmake -S port/r36s -B build/r36s -DCMAKE_BUILD_TYPE=Release
    cmake --build build/r36s --parallel 2

Test without game assets (GX initializes against a real GLES3 context):

    RE4_R36S_LOW_MEM=1 build/r36s/re4_r36s --probe --windowed

On the actual R36S, use the ARM64 binary, matching whole-program
native core, matching REL modules, and original game data you own:

    /roms/ports/cubeevil4/
        re4_r36s
        launch-r36s.sh
        libre4_main.so
        libre4_platform.so
        modules/libre4_*.so
        data/
        logs/

The extracted data directory must match the layout described in
port/whole/runtime/dvd.cpp. No game assets are included.

Test the device's rendering support first:

    sh launch-r36s.sh --probe

Then use sh launch-r36s.sh with the matching full core and data.
This requires real **GLES3** support from the firmware's Linux GPU
driver. GLES2-only systems are not supported by the current GX backend;
a separate renderer would be required. A Mesa software result on
GitHub Actions does not indicate hardware performance on R36S.

Controller: SDL standard A/B/X/Y, left stick movement, right stick C-stick,
shoulder buttons and analog triggers; D-pad and Start. Keyboard development
fallbacks are WASD/arrow keys, J/K/U/I and Enter. If SDL's pad mapping
is incomplete, raw button numbers or mapping fixes may be required.

RE4DC visual LOD, mesh and lighting conversions have NOT been
implemented in this phase. The game's simulation code remains intact.
