# R36S Phase 2: native SDL2/GLES3 frontend

Experimental code: port/r36s/. See its README for compiler setup,
launcher usage, raw exit-button mapping and runtime requirements.

Phase 1 added a low-memory rendering policy to the GX renderer.
Phase 2 provides a Linux SDL2 host for that renderer. The frontend
accepts native game-core GX chunks without Android JNI or full-console
emulation; it only works when the complete native whole-program core
and user-provided game resources are available.

The GitHub Action builds the real GLES3 renderer on x86_64 Linux and
checks initialization under Mesa. A host PASS is neither an ARM64
binary nor a pass on the Mali-G31.

Next gates:
1. Build Linux AArch64 frontend against an ARM64 SDL2/EGL/GLES3 sysroot.
2. Test matching whole-program core ABI, modules, and MEM1 allocation.
3. Run --probe on dArkOSRE and ArkOS4Clone to confirm actual hardware GLES3.
4. Validate input (especially exit), audio, saves, and frame pacing.
5. Measure actual CPU/GPU frame cost and RSS before porting visual-only
   LOD and other RE4DC ideas.

Do not merge the experimental PR to master without device tests.
