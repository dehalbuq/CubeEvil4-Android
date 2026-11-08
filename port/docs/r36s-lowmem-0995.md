# R36S experimental low-memory render profile (CubeEvil4 0.99.5-rc)

## Source and scope

This experimental branch merges `Andiweli/CubeEvil4-Android` at
`cdaa4eb313784afebf5bd2a04bf12e2abb10a258` (tag `0.99.5-rc`)
into this R36S fork. The original R36S AArch64 core CI, RE4DC feasibility
notes and read-only process profiler have been preserved.

The upstream project is a native Android ARM port of the GameCube RE4
decompilation. It is NOT an R36S-ready Linux port. Its rendering frontend
depends on GLES 3 or optional Vulkan, and the current R36S CI still builds
only the AArch64 **core**, not a complete Linux SDL/EGL controller/audio
frontend. No working R36S executable or game data is contained here.

## Opt-in graphics policy

Only a frontend that is **compiled from this updated branch** can honor
the new environment switch:

    RE4_R36S_LOW_MEM=1

The switch is **off by default** and affects presentation only:

| Area | Experimental setting | Motivation |
| --- | --- | --- |
| EFB render target | maximum 1x (640x528) | avoid high-resolution GPU attachments |
| Multisampling | off (1 sample) | prevent multiple color/depth buffers |
| Anisotropy | 1x | reduce texture-sampling cost |
| Framebuffer invalidation, tiled MSAA, vertex ring | off | avoid unqualified driver-specific paths |
| Optional FXAA / sharpen / TV / PSX / lens-flare / strong DoF | off | remove post-processing work |
| Standard shader programs in RAM | at most 256 | reduce driver memory footprint |
| Merged-draw shader programs in RAM | at most 64 | reduce driver memory footprint |
| Original game logic, collisions, AI, save data | unchanged | preserve game behavior |

The original (ordinary) DoF logic is not removed, and original GX draw
streams, meshes, animations, effects and their resource lifetimes remain
unchanged. The cap controls **in-memory shader residency**, not the
total on-disk shader cache size. A smaller cap can cause more shader
reloads and stutter: this is an A/B test, not a guaranteed FPS gain.

The original EFB height is 528; a future Linux presenter should crop or
letterbox correctly for the R36S 640x480 **4:3** LCD and must not
distort pixels or use the EFB height as the display aspect.

Note: exporting this variable with an **older R36S executable does
nothing**, because older binaries do not contain the profile code.

## Tests

Portable no-GL policy unit test:

    g++ -std=c++17 -Wall -Wextra -Werror -O2 \
      -Iport/android-decomp/app/src/main/cpp \
      port/android-decomp/tests/r36s_lowmem_profile_test.cpp \
      -o /tmp/r36s_lowmem_profile_test
    /tmp/r36s_lowmem_profile_test

The branch adds `.github/workflows/r36s-lowmem-profile.yml` with this
test plus Linux host syntax checks for `gx_renderer.cpp` and
`gx_gl.cpp` (libgles2-mesa-dev). This is NOT device or runtime testing.

## RE4DC techniques: what is / is not implemented

RE4DC's documented compromises include static baked lighting, PS2-derived
or reduced visual meshes, simplified transparent materials and water,
shorter visual LOD/draw distance, fewer particles and staged residency.

This first stage reuses their *low-cost presentation principle*, but it
**does not** pretend to import Dreamcast/PVR assets into Mali-G31 or
silently simplify gameplay geometry. Source geometry and source
collision will remain the reference. Visual mesh substitutions require
an offline converter and scene-by-scene comparison; no modified
proprietary assets are included.

## Hardware acceptance gates

Before claiming R36S performance gains, a runnable Linux EGL frontend is
needed. On the R36S DarkOSRE, with the same saved game / room / camera:

1. Compare default and `RE4_R36S_LOW_MEM=1`, using identical input.
2. Confirm GLES 3 context and Mali-G31 hardware rendering (not llvmpipe).
3. Measure FPS at normal **game speed**, 1% lows, CPU/GPU frame times,
   process RSS/high-water mark, and graphics stalls.
4. Check village fight, house interior, inventory, movies, gameplay
   camera, controls, sound and save/load.
5. Use `port/tools/r36s_perf_probe.sh` over SSH to log resource usage,
   independent of the game's broken in-game exit combination.
6. Compare shader cache loads/stutter with caps 256/64 versus default.
   Reject any optimization with broken graphics or timing.

### Follow-up tasks

- Integrate a native Linux SDL2/EGL frontend around updated GLES3 code.
- Verify Mali-G31 driver extensions on each firmware; GLES2 fallback is
  a separate port and does not yet exist for this renderer.
- Profile real GX draws/EFB copies and eliminate only proven redundant
  GPU work.
- Qualify RE4DC-style visual-only LOD/prelit scenery and lower vegetation
  against original actor/collision/interaction state.
- Revisit 320x240 internal rendering only after testing scaled scissor,
  depth, XFB/EFB copies, HUD and text: the current renderer clamps to 1x.

No target FPS is promised. The initial aim is correct play speed and
stable frame times before tuning frame rate upward.
