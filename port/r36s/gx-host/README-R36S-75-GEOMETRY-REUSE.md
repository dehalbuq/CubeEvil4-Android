# R36S Resident Evil 4 — GPU geometry buffer optimization (quality 75%)

## Evidence from the real handheld

The R36S (RK3326 Cortex-A35 / Mali-G31) successfully runs the original
GameCube Resident Evil 4 native recompilation GX renderer in DarkOSRE.
The user prioritizes **480x396** internal render resolution.

A measured 180-second A/B of the previous faststate BP/XF optimization
reported 1872 vs 1873 GX-presented chunks, despite ~4.24 million avoided
redundant XF writes. The evidence suggests the GX register replay CPU
work is NOT the primary FPS limit. Gameplay typically submits roughly
400–530 GLES draw calls and ~100–130k triangles per presented frame.

One expensive source-level operation remained:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glBufferData(GL_ARRAY_BUFFER, frame.vertexBytes, frame.vertices, GL_STREAM_DRAW);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, frame.indexCount*2, frame.indices, GL_STREAM_DRAW);
```

It potentially forces new driver storage and book-keeping on **every
GX chunk**, including chunks with a few megabytes of decoded vertex
data. Because each chunk is followed by a **mandatory and already
validated glFinish** in R36S swap mode 5, the previous data is no
longer GPU-in-flight when the next chunk starts. The R36S-specific
experimental renderer can allocate large-enough storage once and
update content with `glBufferSubData`.

## What this patch actually changes

Two **separate ARM64 real-game executables** are compiled with identical
`-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto`, same game core,
same Mali GLES shader binary cache and exactly the same safe visuals:

- **`r36s_game_a35_geom_base`**: original `glBufferData` every chunk.
- **`r36s_game_a35_geom_reuse`**: reusable vertex/index GL buffers
  (`glBufferData` on first use or capacity growth, followed by
  `glBufferSubData` for each chunk).

The reuse version warms to **8 MiB VBO + 1 MiB IBO**; grows to at most
16 MiB / 4 MiB when needed, and falls back to an exact-size upload for
oversized chunks. It does **not** perform asynchronous unsynchronised
buffer writes and relies on the known stable `glFinish` for safety.
It may still perform **worse** than orphaning each buffer on the Mali
driver: this is an A/B hypothesis, not a claimed FPS gain.

Both targets include the same small instrumentation:

- `GEOMETRY_UPLOAD_MODE`
- `GEOMETRY_UPLOAD_WINDOW FRAMES=... CHUNKS=... BYTES=...
  WALL_US=... AVG_MS_PER_CHUNK=... ALLOC_CALLS=...
  SUBDATA_CALLS=... OVERSIZE_FALLBACKS=...`
- `GEOMETRY_UPLOAD_TOTAL`, `PERFORMANCE_WINDOW_SECONDS`,
  `FPS_PRESENTED`, `SYSTEM_MEMORY_SAMPLE`, front/core RSS,
  shader/GL/IPC errors and process exit code.

The upload wall-clock includes **possible GL driver or GPU wait time**;
it is not a standalone GPU benchmark.

## Fixed image quality, unchanged gameplay and files

- EFB **480×396**; no more scaling down.
- Two dynamic vertex lights per GX channel.
- DOF and fog off (same as the already-used Ultra Seguro).
- No camera-distance culling; no vertex or triangle dropped;
  no unsafe simplification of vegetation, enemies, collisions or items.
- Shader disk cache ON, BaseVertex OFF, SDL KMSDRM
  VSync1 + **glFinish before SDL_GL_SwapWindow**.
- Keeps `r36s_game_boot`, `RE4-R36S-Estavel.sh`,
  `r36s_game_a35_scalable`, `re4_coreproc`, libs,
  all game data, modules and memory card untouched.

## Installation on DarkOSRE

Download GitHub Actions artifact:
**`CubeEvil4-R36S-75pct-GX-Buffer-Reuse-ARM64`**.

Extract its contents into `/roms/ports`, **without deleting any
existing game files**. It adds two new executables under
`cubeevil4_live_gx/` and three independent Ports scripts.

1. **`RE4-R36S-75-Buffer-Original.sh`** — run 180 seconds,
   enter the first playable area, ideally perform a repeatable route.
2. **`RE4-R36S-75-Buffer-Reutilizavel.sh`** — repeat same scene,
   camera movements and play duration for an apples-to-apples A/B.
3. Observe visual quality: XFB, 3D surfaces, background, transparency,
   shadows, items, HUD, controllers and screen transitions.
4. Send BOTH logs:

```
/roms/ports/cubeevil4_live_gx/diagnostico_a35_75_geometria_base_darkos.txt
/roms/ports/cubeevil4_live_gx/diagnostico_a35_75_geometria_reuse_darkos.txt
```

If the alternative does not increase FPS, or it causes graphics issues,
do not adopt it. Return to `RE4-R36S-Qualidade-75.sh` or to the
maximum-compatible `RE4-R36S-Estavel.sh`. Back up
`data/memcard/card-a.bin` before experimental sessions.

## Why not lower polygons or remove trees now?

Most GX draw state is already cached and the game's GX decoder already
coalesces ~16,000 raw GXBegin primitives into ~470 actual draw calls
per typical forest frame. Cutting polygons could lower vertex upload
cost, but may not lower the number of draw calls, and would require
precise model identification/LOD integration. The user requested a
code-only iteration, leaving the offline model tooling for later.

**This package contains no copyrighted game files.**
