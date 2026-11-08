# CubeEvil4 0.99.5 RC – experimental native R36S frontend

This is **not a verified playable R36S release**. The existing SDL2
AArch64 IPC frontend and physical-button evdev exit fix have been
recovered from prior R36S work and adapted to the newer GX renderer.
Nothing is changed on master.

Native roles:
- `re4_r36s`: SDL2 + GLES3/GX compatibility **probe only**.
- `r36s_game_showcase_evdev_exit`: original GX stream presenter plus
  controller and SDL audio, using separate `re4_coreproc`.
- `re4_coreproc`: separate Linux AArch64 game-core process,
  reserving high virtual addresses before loading game libraries.

The full experimental artifact is built by GitHub Actions
`r36s-linux-frontend.yml` ONLY when both the native AArch64
frontend and the matching whole-program AArch64 core finish
successfully. It contains no ISO, original extracted assets, saves
or licensed artwork. Older R36S installations use a different folder.

Layout after successful CI packaging:

    /roms/ports/RE4-R36S-0995-EXPERIMENTAL.sh
    /roms/ports/cubeevil4_0995_exp/
      re4_r36s
      r36s_game_showcase_evdev_exit
      r36s_showcase_input_probe
      re4_coreproc
      libre4_main.so
      libre4_platform.so
      libre4_host.so
      modules/libre4_*.so
      launch-r36s.sh
      data/disc0/  # YOU supply original extracted game data
      logs/

Instructions:
1. Test `cd /roms/ports/cubeevil4_0995_exp`.
2. Run `sh launch-r36s.sh --probe`.
3. Read `logs/r36s-upstream0995-last.log`. Hardware GLES3 required;
   if GPU reads llvmpipe/software Mesa, performance cannot be tested.
4. Once the matching original data is installed, launch with
   `sh launch-r36s.sh`, or the Ports entry.

Controls and exit:
- Gameplay uses SDL2 controller mapping.
- Physical Linux `BTN_START`/`BTN_SELECT` (via readonly evdev)
  supplement SDL on clone firmware when permissions allow.
- Start+Select 2 seconds, or Start 8 seconds requests exit.
- `killall -USR1 r36s_game_showcase_evdev_exit` is an SSH fallback
  for an otherwise responsive game loop.

The only supported low-memory switch is `RE4_R36S_LOW_MEM=1`:
1x GX EFB, MSAA off, post effects off, limited shader residency.
Old A35/PS2 graphics macros or scale values in historical scripts
are not guaranteed to do anything against the updated renderer.
Do not promise 30 FPS, 60 FPS, correct gameplay or working saves.
The RE4DC LOD/lighting/material experiments are a future separate
visual-only effort, requiring hardware profiling.

Developers on Linux:
    cmake -S port/r36s -B build/r36s -DCMAKE_BUILD_TYPE=Release
    cmake --build build/r36s -j2
    RE4_R36S_LOW_MEM=1 build/r36s/re4_r36s --probe --windowed

For the complete ARM64 game executable, use the CI artifacts from
the same source revision. No Android Java/JNI code is required.


## 0.99.5 native visual reduction (real core option)

The executable now calls the current upstream `re4ipc_set_potato(mask)`
API, not a removed legacy R36S graphics define. The installation script
defaults to `RE4_R36S_POTATO_MASK=3` (disable weather and special
visual effects during ordinary gameplay). The original game rules,
collision and AI are not changed by this rendering switch.

For a controlled A/B test, override before launch:

    RE4_R36S_POTATO_MASK=0 sh launch-r36s.sh    # all original visual effects
    RE4_R36S_POTATO_MASK=3 sh launch-r36s.sh    # low: weather + FX disabled
    RE4_R36S_POTATO_MASK=7 sh launch-r36s.sh    # fewer particles too
    RE4_R36S_POTATO_MASK=31 sh launch-r36s.sh   # aggressive visual reductions

Bits: 1 weather; 2 special effects; 4/8 particle quality (2-bit
level); 16 additional light/shadow effects. The upstream core keeps
effects in cutscenes and events. This is an **unverified performance
experiment**, not evidence that the game achieves a target FPS.
