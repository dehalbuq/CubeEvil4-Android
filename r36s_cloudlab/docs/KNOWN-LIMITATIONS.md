# Known limitations of the CloudLab

This is not a real image of dArkOSRE and not a Switch emulator. It is a
reproducible ARM64 Linux compatibility laboratory.

* **Kernel ABI:** a container always uses its host's kernel, not R36S 4.4.189;
  Linux syscalls and driver ioctl behavior may differ. QEMU-system boot of the
  actual kernel/DTB could improve system-level fidelity but still lacks GPU.
* **CPU:** ARM64 instructions and AAPCS64 ABI run natively on Ubuntu ARM64;
  the hosted CPU is NOT the RK3326 Cortex-A35 in speed, caches, or extensions.
  Compile production binaries with `-mcpu=cortex-a35` and check ELF ISA use.
* **GPU:** Mesa llvmpipe/OpenGL ES 3 exercises the GLES API and shader
  compilation; cannot reproduce Mali-G31 r13p0 timing or proprietary driver bugs.
* **RAM:** the reported **usable Linux total is 895 MiB**, with approximately
  **695 MiB available at collection time** and no swap. 768 MiB in RE4NX Phase 3
  is a **synthetic virtual address-bookkeeping ceiling**, not guaranteed usable RSS.
  New tests touch and read back **64, 256, 512 MiB**; 640 MiB is an optional
  stress ceiling, not a confirmed stable working set. Docker `--memory=895m
  --memory-swap=895m` uses Linux cgroups, but does not reproduce shared VRAM,
  fragmentation, Rockchip CMA reservations, cache behavior, or DRAM bandwidth.
* **Disk/filesystem:** host storage does not reproduce microSD latency or
  file permission mappings from the dArkOSRE build.
* **Input:** synthetic SDL mapping may be tested, not actual RK29 GPIO evdev.

**Privacy:** Never upload a full root filesystem, `Nintendo/` files, game
`data/disc0`, game images, save files, credentials, keys, MAC/IPs or system
unique IDs to public GitHub repositories. This collector avoids them by default.
Optional --with-libs snapshots can include proprietary driver libraries, so
review licensing and only share these privately if distribution is permitted.

* **Userspace:** collector identifies Debian 13 (trixie) and SDL2 2.32.x,
  while the native CI runner uses Ubuntu 24.04 and the cgroup container uses
  Debian trixie. A container shares the **host kernel**, so this does not
  recreate the R36S kernel or privileged device-driver interfaces.
* **Clock:** archive filename is dated September 2025 while the recorded
  kernel build reports December 2025. Do not use the handheld clock as a
  trusted source for build ordering or reproducible results.
* **Input:** neither the device name nor the existence of event nodes proves
  that Start/Select presses reach SDL. The older diagnostic did not capture
  per-button DOWN/UP transitions, so physical mapping is still unverified.
* **Commercial RE4:** NO Switch code executes. Synthetic NSO0 tests and a
  simulated IPC table cannot establish compatibility with the commercial game.
