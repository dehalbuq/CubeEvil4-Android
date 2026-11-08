# CubeEvil4 0.99.5-rc – R36S experimental ARM64 IPC

**Status: source integration and CI configuration, NOT a successful
R36S gameplay test.** The original working experiments are preserved
in `chatgpt/r36s-exit-evdev-independent-of-sdl` and the
`master` branch is unchanged.

## What changed

- Restored the existing native R36S SDL2/GLES3 game host and real Linux
  evdev Start/Select detection from the October work into
  `port/r36s/gx-host/` *as a separate directory*.
- Adapted its CMake builds to the updated GLES3 GX backend:
  `gx_gl.cpp`, `gx_core.cpp`, `gx_renderer.cpp`,
  `gx_gl_shader.cpp`, `gx_texture.cpp`.
- Removed the obsolete `SetShowcaseDisplayCrop` call and the old
  pre-0.99.5 `RE4_R36S_LOW_SPEC` compilation option; do not mistake
  old rendering switches for the new low-memory profile.
- Enabled the separate `re4_coreproc` for Linux AArch64 and
  restored its high-address reservation before library loading. This
  keeps GX/SDL process memory distinct from game MEM1 mappings.
- Added the native `re4_r36s` *probe*, intentionally with NO direct
  `dlopen` game startup: loading a 32-bit-pointer-sensitive game core
  inside an existing SDL process is not a safe default.
- Added an isolated launch script and Actions workflow to compile the
  updated ARM64 whole-program core + host from the same source revision.

## What is safe to test

GitHub Actions workflow: `.github/workflows/r36s-linux-frontend.yml`.
If all jobs succeed, download artifact
`CubeEvil4-R36S-0995-EXPERIMENTAL-ARM64`.

Expected files under `/roms/ports/`:

    RE4-R36S-0995-EXPERIMENTAL.sh
    cubeevil4_0995_exp/
      re4_r36s                       # GLES3 hardware probe ONLY
      r36s_game_showcase_evdev_exit  # actual game GX IPC frontend
      r36s_showcase_input_probe
      re4_coreproc                   # Linux ARM64 separate core process
      libre4_main.so
      libre4_platform.so
      libre4_host.so
      modules/libre4_*.so
      launch-r36s.sh
      README-R36S.txt

Copy your own extracted original GameCube data to
`cubeevil4_0995_exp/data/disc0/` (including `sys/boot.bin`,
`sys/fst.bin`, and `files/`). No game data, ISO, saves or original
art are bundled. The new folder is intentionally **different** from
`cubeevil4_live_gx`, preserving older A35/O3 experiments and memory
cards. The data directory must match the supported title/revision.

Before running gameplay, from SSH or terminal run:

    cd /roms/ports/cubeevil4_0995_exp
    sh launch-r36s.sh --probe
    cat logs/r36s-upstream0995-last.log

Check `R36S_PROBE_RESULT=PASS` and the **actual GPU** (not llvmpipe
or software Mesa). To try gameplay, run `sh launch-r36s.sh`.
Log output is in `logs/r36s-upstream0995-last.log`.

Physical exit: hold Start+Select for 2 seconds, or Start for
8 seconds; this uses the restored evdev physical button reader
where permitted, not guessed raw button indices. If the device
does not expose or permit these codes, the exit combination may
still fail. Via SSH, `killall -USR1 r36s_game_showcase_evdev_exit`
requests safe exit of a responsive frontend.

## Expected limitations

- GLES **3+**, not GLES2 fallback. Actual firmware driver compatibility
  is unknown. Old GLES2-only systems will not work.
- Frame rate and game speed have not been measured. The low-memory
  profile `RE4_R36S_LOW_MEM=1` avoids heavy MSAA and post effects,
  caps shader residency, and retains the 1x EFB size. An older 75%
  or 50% EFB switch in the recovered frontend no longer implies that
  scaling works with the updated 0.99.5 renderer.
- Old RE4DC-style geometry/LOD/lighting optimizations have not been
  imported. All meshes/collision and original simulation remain intact.
- CI compilation / Mesa GX initialization / hardware execution
  are different gates. The Actions workflow must pass before a
  compiled artifact can be considered available.
- Confirm real hardware RAM, input, audio, game speed, crashes,
  picture aspect, saves and GPU GL errors before ever merging PR #29.


## 0.99.5 native visual reduction (real core option)

The executable now calls the current upstream `re4ipc_set_potato(mask)`
API, not a removed legacy R36S graphics define. The installation script
defaults to `RE4_R36S_POTATO_MASK=31` (aggressive reduction of weather,
special effects, particles, and optional light/shadow effects during ordinary gameplay). The original game rules,
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
