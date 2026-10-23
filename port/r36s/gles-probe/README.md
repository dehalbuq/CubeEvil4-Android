# R36S / DarkOSRE: OpenGL ES visual smoke test

This is a graphics diagnostic, **not** a playable Resident Evil 4 port. It checks whether the RK3326 Mali-G31 can create an SDL2 OpenGL ES 3 context using KMSDRM.

## Running

Copy the C file and launcher into a writable folder on /roms/ports and run:

    sh RE4-GLES-Test.sh

The launcher compiles the small test with the device's GCC (no SDL development headers), and loads SDL2 at runtime. It does not install packages, change system drivers, or require a game ISO.

The window cycles blue, green and red. The file diagnostico_gles_darkos.txt in the same folder includes SDL video drivers, GL_VENDOR, GL_RENDERER, GL_VERSION and diagnostic errors.

If an SSH session cannot acquire DRM master while EmulationStation is running, launch it from the Ports menu instead. The logger itself does not stop or restart system services.

## Results

- GLES3_DRAW_OK: an ES3 context drew and swapped frames; inspect GL_RENDERER to confirm hardware acceleration.
- GLES3_NOT_AVAILABLE: ES2 rendered but ES3 did not.
- NO_GL_CONTEXT / SDL failure: check the log for EGL/SDL backend problems.
- A working test does not prove the CubeEvil4 Android renderer works on Linux, nor that the game fits within 1 GB RAM.

This experiment deliberately avoids making a full-game boot claim.
