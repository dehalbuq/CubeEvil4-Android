#!/bin/sh
# CubeEvil4 R36S: read-only performance and compatibility probe.
# Run from SSH before starting the game. Output survives forced game exit.
# Usage: sh r36s_perf_probe.sh [duration_seconds] [interval_seconds]
set -u

DURATION=${1:-180}
INTERVAL=${2:-2}
case "$DURATION" in ''|*[!0-9]*) echo 'duration must be integer' >&2; exit 2;; esac
case "$INTERVAL" in ''|*[!0-9]*) echo 'interval must be integer' >&2; exit 2;; esac
if [ "$DURATION" -lt 1 ] || [ "$DURATION" -gt 3600 ] ||
   [ "$INTERVAL" -lt 1 ] || [ "$INTERVAL" -gt 30 ]; then
    echo 'duration 1..3600s, interval 1..30s' >&2
    exit 2
fi

if [ -n "${RE4_R36S_LOG_DIR:-}" ]; then
    LOG_DIR=$RE4_R36S_LOG_DIR
elif [ -d /roms/ports ]; then
    LOG_DIR=/roms/ports/cubeevil4-diagnostics
else
    LOG_DIR="$PWD/cubeevil4-diagnostics"
fi
umask 077
mkdir -p "$LOG_DIR" || exit 1
STAMP=$(date '+%Y%m%d-%H%M%S' 2>/dev/null || echo unknown)
LOG="$LOG_DIR/r36s-perf-$STAMP-$$.log"
CSV="$LOG_DIR/r36s-process-$STAMP-$$.csv"
exec 3>>"$LOG" || exit 1
printf 'timestamp,pid,comm,rss_kib,peak_rss_kib,threads,cpu_ticks,cpu0_khz,thermal_millic\n' > "$CSV" || exit 1

say() { printf '%s\n' "$*" >&3; }
get_value() { awk -v k="$2" '$1 == k":" {print $2;exit}' "$1" 2>/dev/null; }
thermal() {
    for f in /sys/class/thermal/thermal_zone*/temp; do
        [ -r "$f" ] || continue
        IFS= read -r v < "$f" || :
        case "$v" in ''|*[!0-9]*) continue;; esac
        printf '%s' "$v"; return
    done
    printf NA
}
frequency() {
    f=/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
    if [ -r "$f" ]; then cat "$f" 2>/dev/null; else printf NA; fi
}

say '==== CubeEvil4 R36S read-only preflight ===='
say "date=$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || echo unknown)"
say "duration_s=$DURATION interval_s=$INTERVAL"
say "uname=$(uname -a 2>/dev/null || echo unavailable)"
say "kernel_arch=$(uname -m 2>/dev/null || echo unavailable)"
say "userspace_bits=$(getconf LONG_BIT 2>/dev/null || echo unavailable)"
say "clock_ticks_per_second=$(getconf CLK_TCK 2>/dev/null || echo unavailable)"
if [ -r /etc/os-release ]; then
    grep -E '^(NAME|VERSION_ID|PRETTY_NAME)=' /etc/os-release >&3 2>/dev/null || :
fi
if [ -r /proc/meminfo ]; then
    grep -E '^(MemTotal|MemAvailable|SwapTotal|SwapFree):' /proc/meminfo >&3 2>/dev/null || :
fi
if [ -r /proc/cpuinfo ]; then
    grep -iE '^(Hardware|model name|CPU architecture|Features|processor)[[:space:]]*:' /proc/cpuinfo | head -25 >&3 2>/dev/null || :
fi
for path in /dev/dri/renderD128 /dev/dri/card0 /dev/mali0 /dev/fb0 /dev/input/event0; do
    [ -e "$path" ] && say "device_present=$path"
done
for c in es2_info es3_info eglinfo glxinfo; do
    if command -v "$c" >/dev/null 2>&1; then say "tool_present=$c"; fi
done
say 'gles_libraries:'
for f in /usr/lib/*/libGLESv2.so* /usr/lib/libGLESv2.so* /usr/lib/*/libEGL.so* /usr/lib/libEGL.so*; do
    [ -e "$f" ] && ls -l "$f" >&3 2>/dev/null || :
done
say 'gpu_devfreq:'
for f in /sys/class/devfreq/*/name /sys/class/devfreq/*/governor; do
    [ -r "$f" ] || continue
    printf '%s=' "$f" >&3
    cat "$f" >&3 2>/dev/null || :
done
say 'NOTE: library/tool existence alone does NOT prove a working GLES3 context.'
say 'NOTE: cpu_ticks is cumulative user+kernel CPU time; compare sample deltas.'

PROCESS_NAMES=${RE4_R36S_PROCESS_NAMES:-'re4_coreproc re4_host cubeEvil4 CubeEvil4'}
i=0
while [ "$i" -lt "$DURATION" ]; do
    now=$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || echo unknown)
    freq=$(frequency)
    temp=$(thermal)
    if [ -r /proc/meminfo ]; then
        avail=$(awk '$1=="MemAvailable:" {print $2;exit}' /proc/meminfo 2>/dev/null)
    else
        avail=NA
    fi
    say "sample_s=$i time=$now available_kib=${avail:-NA} cpu0_khz=$freq thermal_millic=$temp"
    found=0
    for p in /proc/[0-9]*; do
        pid=${p##*/}
        [ -r "$p/comm" ] && [ -r "$p/status" ] && [ -r "$p/stat" ] || continue
        IFS= read -r comm < "$p/comm" || continue
        matched=0
        for name in $PROCESS_NAMES; do [ "$comm" = "$name" ] && matched=1; done
        [ "$matched" -eq 1 ] || continue
        found=1
        rss=$(get_value "$p/status" VmRSS)
        peak=$(get_value "$p/status" VmHWM)
        threads=$(get_value "$p/status" Threads)
        # After removing pid and (comm), utime/stime are positions 12/13.
        raw=$(sed 's/^[^)]*)[[:space:]]*//' "$p/stat" 2>/dev/null) || raw=''
        ticks=$(printf '%s\n' "$raw" | awk '{print $12+$13}' 2>/dev/null)
        printf '%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "$now" "$pid" "$comm" "${rss:-0}" "${peak:-0}" "${threads:-0}" "${ticks:-0}" "${freq:-NA}" "$temp" >> "$CSV"
        say "process=$comm pid=$pid VmRSS_kib=${rss:-0} VmHWM_kib=${peak:-0} threads=${threads:-0} cpu_ticks=${ticks:-0}"
    done
    [ "$found" -eq 1 ] || say 'process=not_found (start game or set RE4_R36S_PROCESS_NAMES)'
    sleep "$INTERVAL" || break
    i=$((i + INTERVAL))
done
say 'end=normal'
printf 'Diagnostic saved:\n%s\n%s\n' "$LOG" "$CSV"
