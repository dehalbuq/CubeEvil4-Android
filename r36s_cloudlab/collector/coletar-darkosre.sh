#!/bin/sh
# dArkOSRE/R36S passive diagnostics, portable POSIX sh (BusyBox compatible).
# Does NOT collect game contents, keys, saves, firmware blobs or personal IDs.
set -u
VERSION='1.0'
WITH_LIBS=0
WITH_PORT_LOGS=0
for arg in "$@"; do
 case "$arg" in
  --with-libs) WITH_LIBS=1 ;;
  --with-port-logs) WITH_PORT_LOGS=1 ;;
  --help|-h) echo 'Uso: sh R36S-Coletar-DarkOSRE.sh [--with-libs] [--with-port-logs]'; exit 0 ;;
  *) echo "ERRO: opcao desconhecida: $arg" >&2; exit 2 ;;
 esac
done
umask 077
# Place the archive next to the user's Ports directory, not in /tmp or a game folder.
if [ -n "${R36S_CLOUDLAB_OUT:-}" ] && [ -d "$R36S_CLOUDLAB_OUT" ] && [ -w "$R36S_CLOUDLAB_OUT" ]; then
 PARENT=$R36S_CLOUDLAB_OUT
elif [ -d /roms/ports ] && [ -w /roms/ports ]; then
 PARENT=/roms/ports
elif [ -w "$(pwd)" ]; then
 PARENT=$(pwd)
else
 echo 'ERRO: pasta de destino sem permissao de escrita' >&2; exit 1
fi
TMP=$(mktemp -d "$PARENT/.darkosre-cloudlab-XXXXXXXX") || exit 1
# It only removes the private temporary directory created above.
trap 'rm -rf "$TMP"' 0
trap 'exit 130' 1 2 3 15
OUT="$TMP/darkosre-profile"
mkdir -p "$OUT/os" "$OUT/cpu" "$OUT/gpu" "$OUT/ram" "$OUT/input" "$OUT/libs" "$OUT/ports" "$OUT/toolchain" "$OUT/kernel"
LOG="$OUT/collection-status.txt"
printf 'COLLECTOR_VERSION=%s\n' "$VERSION" > "$LOG"
printf 'WITH_LIBS=%s\nWITH_PORT_LOGS=%s\n' "$WITH_LIBS" "$WITH_PORT_LOGS" >> "$LOG"
command_exists() { command -v "$1" >/dev/null 2>&1; }
read_safe() {
 # $1 = absolute source file; $2 = relative destination under OUT
 [ -r "$1" ] || return 0
 cat "$1" > "$OUT/$2" 2>/dev/null || :
}
capture() {
 # Only fixed, known commands supplied by this script, never user text.
 dest=$1; shift
 { "$@"; } > "$OUT/$dest" 2>&1 || printf 'OPTIONAL_COMMAND_FAILED=%s\n' "$*" >> "$LOG"
}
get_num() {
 val=$1
 case "$val" in ''|*[!0-9]*) echo 0;; *) echo "$val";; esac
}
KERNEL=$(uname -r 2>/dev/null || echo unknown)
ARCH=$(uname -m 2>/dev/null || echo unknown)
RAM_KB=$(awk '/^MemTotal:/ {print $2;exit}' /proc/meminfo 2>/dev/null || echo 0)
RAM_KB=$(get_num "$RAM_KB")
RAM_MIB=$((RAM_KB / 1024))
CPUS=$(grep -c '^processor[[:space:]]*:' /proc/cpuinfo 2>/dev/null || echo 0)
CPUS=$(get_num "$CPUS")
STAMP=$(date '+%Y%m%d-%H%M%S' 2>/dev/null || echo unknown)
cat > "$OUT/profile.env" <<PROFILE
PROFILE_VERSION=1
ARCH=$ARCH
KERNEL_RELEASE=$KERNEL
RAM_TOTAL_MIB=$RAM_MIB
CPU_LOGICAL=$CPUS
GPU_TARGET=MALI_G31_EXPECTED_VERIFY_FROM_REPORT
GLES_TARGET=3
GPU_PERFORMANCE_CLOUD_EQUIVALENT=NO
PROFILE
capture os/uname.txt uname -srmv
read_safe /etc/os-release os/os-release.txt
read_safe /etc/issue os/issue.txt
read_safe /proc/version kernel/version.txt
read_safe /proc/loadavg cpu/loadavg.txt
read_safe /proc/uptime os/uptime.txt
# /proc/cpuinfo may contain unique 'Serial' or 'Hardware ID': remove them.
if [ -r /proc/cpuinfo ]; then
 grep -Eiv '^[[:space:]]*(Serial|Unique|MAC|Ethernet|Machine[[:space:]]*ID)[[:space:]]*:' /proc/cpuinfo > "$OUT/cpu/cpuinfo-redacted.txt" || :
fi
read_safe /proc/meminfo ram/meminfo.txt
read_safe /proc/swaps ram/swaps.txt
read_safe /proc/pressure/memory ram/pressure-memory.txt
read_safe /proc/pressure/cpu cpu/pressure-cpu.txt
read_safe /sys/kernel/mm/transparent_hugepage/enabled ram/thp.txt
capture ram/ulimit.txt sh -c 'ulimit -a'
if command_exists free; then capture ram/free-m.txt free -m; fi
if command_exists getconf; then
 capture cpu/page-size.txt getconf PAGESIZE
 capture cpu/nproc-onln.txt getconf _NPROCESSORS_ONLN
fi
if command_exists lscpu; then capture cpu/lscpu.txt lscpu; fi
# Device-tree model and compatibility ONLY. Never collect /serial-number or MAC.
for path in /proc/device-tree/model /sys/firmware/devicetree/base/model; do
 if [ -r "$path" ]; then tr '\000' '\n' < "$path" > "$OUT/cpu/device-model.txt"; break; fi
done
for path in /proc/device-tree/compatible /sys/firmware/devicetree/base/compatible; do
 if [ -r "$path" ]; then tr '\000' '\n' < "$path" > "$OUT/cpu/device-compatible.txt"; break; fi
done
for item in /sys/devices/system/cpu/cpu[0-9]*; do
 [ -d "$item" ] || continue
 c=${item##*/}; mkdir -p "$OUT/cpu/$c"
 for field in scaling_cur_freq scaling_max_freq scaling_min_freq scaling_available_frequencies scaling_governor cpuinfo_max_freq cpuinfo_min_freq; do
  read_safe "$item/cpufreq/$field" "cpu/$c/$field.txt"
 done
done
# Thermal sensors and GPU DVFS (temperatures/frequencies only; no serials).
for item in /sys/class/thermal/thermal_zone[0-9]*; do
 [ -d "$item" ] || continue
 t=${item##*/}; mkdir -p "$OUT/cpu/thermal/$t"
 for field in type temp policy; do read_safe "$item/$field" "cpu/thermal/$t/$field.txt"; done
done
for item in /sys/class/devfreq/*; do
 [ -d "$item" ] || continue
 d=${item##*/}
 # Device names can include punctuation; store under monotonically numbered file.
 {
  printf 'DEVFREQ_DEVICE=%s\n' "$d"
  for field in name governor cur_freq min_freq max_freq available_frequencies trans_stat; do
   if [ -r "$item/$field" ]; then
    printf '\n[%s]\n' "$field"
    head -c 32768 "$item/$field" 2>/dev/null || :
    printf '\n'
   fi
  done
 } >> "$OUT/gpu/devfreq.txt"
done
for item in /sys/class/drm/card[0-9]*; do
 [ -e "$item" ] || continue
 printf '%s\n' "${item##*/}" >> "$OUT/gpu/drm-nodes.txt"
 for prop in status modes enabled; do
  if [ -r "$item/$prop" ]; then
   printf '[%s %s]\n' "${item##*/}" "$prop" >> "$OUT/gpu/drm-output.txt"
   head -c 4096 "$item/$prop" >> "$OUT/gpu/drm-output.txt" 2>/dev/null || :
   printf '\n' >> "$OUT/gpu/drm-output.txt"
  fi
 done
done
if [ -d /dev/dri ]; then capture gpu/dev-dri.txt ls -ln /dev/dri; fi
for node in /dev/mali0 /dev/mali /dev/ump; do
 [ -e "$node" ] && ls -ln "$node" >> "$OUT/gpu/gpu-nodes.txt" 2>/dev/null || :
done
# Explicit capability metadata; no raw input events or personal key presses.
if [ -r /proc/bus/input/devices ]; then
 grep -E '^(I:|N:|H:|B:)' /proc/bus/input/devices > "$OUT/input/devices-redacted.txt" || :
fi
if [ -d /dev/input ]; then capture input/input-node-names.txt ls -ln /dev/input; fi
# Installed compiler/runtime capabilities useful for reproducing the ABI.
for prog in sh busybox gcc g++ clang cmake make ninja readelf objdump ldd lscpu eglinfo es2_info glxinfo sdl2-config pkg-config python3; do
 if command_exists "$prog"; then
  (command -v "$prog") >> "$OUT/toolchain/available-commands.txt"
 fi
done
if command_exists gcc; then capture toolchain/gcc.txt gcc --version; fi
if command_exists clang; then capture toolchain/clang.txt clang --version; fi
if command_exists sdl2-config; then capture toolchain/sdl2-version.txt sdl2-config --version; fi
if command_exists pkg-config; then
 capture toolchain/sdl2-pkg-version.txt pkg-config --modversion sdl2
 capture toolchain/egl-pkg-version.txt pkg-config --modversion egl
 capture toolchain/glesv2-pkg-version.txt pkg-config --modversion glesv2
fi
if command_exists ldconfig; then
 (ldconfig -p 2>/dev/null || :) | grep -Ei 'lib(SDL2|EGL|GLES|mali|drm|gbm|udev|asound|vulkan|c\.so|stdc\+\+)' > "$OUT/libs/ldconfig-filtered.txt" || :
fi
# Read-only EGL/GLES probe; avoid hanging the game launcher for too long.
if command_exists timeout; then
 if command_exists eglinfo; then capture gpu/eglinfo.txt timeout 8 eglinfo -B; fi
 if command_exists es2_info; then capture gpu/es2-info.txt timeout 8 es2_info; fi
else
 printf 'EGLINFO_SKIPPED=timeout_command_unavailable\n' >> "$LOG"
fi
# Library hashes, types and ABI. No library files by default.
LIBCOUNT=0
: > "$OUT/libs/library-fingerprint.tsv"
for base in /usr/lib /lib /usr/local/lib; do
 [ -d "$base" ] || continue
 # Name list is restricted to GPU/render/audio/input libraries.
 find "$base" -maxdepth 4 \( -type f -o -type l \) \
  \( -name 'libmali*.so*' -o -name 'libEGL*.so*' -o -name 'libGLES*.so*' \
   -o -name 'libSDL2*.so*' -o -name 'libdrm*.so*' -o -name 'libgbm*.so*' \
   -o -name 'libasound*.so*' \) -print 2>/dev/null || :
done | sort -u | while IFS= read -r p; do
 [ -r "$p" ] || continue
 if command_exists sha256sum; then hash=$(sha256sum "$p" 2>/dev/null | awk '{print $1}'); else hash=NO_SHA256SUM; fi
 size=$(wc -c < "$p" 2>/dev/null | tr -dc '0-9')
 [ -n "$size" ] || size=0
 printf '%s\t%s\t%s\n' "$hash" "$size" "$p" >> "$OUT/libs/library-fingerprint.tsv"
 if [ "$WITH_LIBS" -eq 1 ]; then
  # Opt-in snapshot: only selected system libraries, never /roms/ports.
  # Skip >64 MiB blobs. Preserve path, dereference symlink to avoid escaping.
  if [ "$size" -le 67108864 ]; then
   realp=$p
   if command_exists readlink; then realp=$(readlink -f "$p" 2>/dev/null || echo "$p"); fi
   case "$realp" in
    /usr/lib/*|/lib/*|/usr/local/lib/*)
     mkdir -p "$OUT/sysroot$(dirname "$p")"
     cp -L "$p" "$OUT/sysroot$p" 2>/dev/null || :
     ;;
    *) printf "SYMLINK_TARGET_OUTSIDE_SYSTEM_LIBS_SKIPPED=%s\n" "$p" >> "$LOG" ;;
   esac
  fi
 fi
done
# Log extraction is explicit, capped, and limited to known diagnostic logs.
if [ "$WITH_PORT_LOGS" -eq 1 ]; then
 for f in /roms/ports/re4nx_r36s_prototype/logs/ultimo-re4nx-r36s.log \
          /roms/ports/cubeevil4_live_gx/logs/ultimo_inicio_agressivo.txt \
          /roms/ports/cubeevil4_live_gx/logs/r36s-upstream0995-last.log; do
  if [ -f "$f" ]; then
   size=$(wc -c < "$f" | tr -dc '0-9')
   if [ "${size:-0}" -le 1048576 ]; then
    cp "$f" "$OUT/ports/$(basename "$f")" 2>/dev/null || :
   fi
  fi
 done
fi
# The bundle is never uploaded automatically. Output is private until shared.
cat > "$OUT/AVISO-LEIA.txt" <<'NOTICE'
R36S dArkOSRE CloudLab - diagnostico, sem upload automatico.
Nao contem jogos, saves, dumps completos da NAND, chaves ou firmware.
Revisar o ZIP antes de compartilhar. Pode conter nomes tecnicos de dispositivos,
caminhos locais e versoes de bibliotecas.
Opcao --with-libs copia bibliotecas de sistema (inclusive GPU proprietaria):
compartilhe-as somente se os termos de licenca permitirem, e NUNCA envie
esse sysroot para repositorios publicos.
Teste em nuvem NAO reproduz velocidade/driver Mali-G31 do hardware real.
NOTICE
if command_exists sha256sum; then
 (cd "$OUT" && find . -type f ! -name SHA256SUMS -print | LC_ALL=C sort | while IFS= read -r f; do sha256sum "$f"; done) > "$OUT/SHA256SUMS"
fi
ARCHIVE="$PARENT/darkosre-cloudprofile-$STAMP.zip"
if command_exists zip; then
 (cd "$TMP" && zip -q -r "$ARCHIVE" darkosre-profile) || exit 1
else
 ARCHIVE="$PARENT/darkosre-cloudprofile-$STAMP.tar.gz"
 (cd "$TMP" && tar -czf "$ARCHIVE" darkosre-profile) || exit 1
fi
printf 'CLOUDLAB_COLLECT=PASS\nARCHIVE=%s\nARCH=%s\nRAM_TOTAL_MIB=%s\n' "$ARCHIVE" "$ARCH" "$RAM_MIB"
printf 'Nenhum dado foi enviado automaticamente. Confira o arquivo antes de compartilhar.\n'
