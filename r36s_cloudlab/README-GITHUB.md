# RE4NX R36S CloudLab — GitHub integration

The source in this folder contains synthetic Phase 1-3 RE4NX smoke tests, a dArkOSRE hardware profile, an offline collector and reproducible CMake/Docker runners. No commercial Switch executable or key is present.

Run on native ARM64 Linux:

```sh
sh r36s_cloudlab/cloud/run-cloud-tests.sh
```

GitHub Actions workflow: `.github/workflows/darkosre-cloudlab.yml`.
It checks AArch64 CPU execution, NSO0 parsing, synthetic Horizon-like IPC, memory/W^X and Mesa software EGL/GLES3. The Docker check uses a 895 MiB RAM cap and no swap.

IMPORTANT: Cloud results do not establish R36S Mali-G31 FPS or compatibility with actual Nintendo Switch games. Review technical metadata in docs before reusing this branch publicly.
