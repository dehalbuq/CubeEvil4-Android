#!/bin/sh
# Resident Evil 4 on R36S DarkOSRE: opt-in PS2-inspired low-quality settings.
# Native EGL/GLES stable swap mode 5 retained; stable game binary untouched.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
CORE="$HERE/cubeevil4_live_gx"
DATA="$CORE/data"
PROFILE="${RE4_R36S_PS2_PRESET:-1}"
case "$PROFILE" in
  1)
    NAME="balanceado"
    LIGHTS=2; NO_DOF=1; NO_FOG=0; FAR_Z=0
    ;;
  2)
    NAME="ps2"
    LIGHTS=1; NO_DOF=1; NO_FOG=0; FAR_Z=90
    ;;
  3)
    NAME="ultra"
    LIGHTS=0; NO_DOF=1; NO_FOG=1; FAR_Z=45
    ;;
  *)
    echo "Invalid RE4_R36S_PS2_PRESET=$PROFILE (1=balanced 2=PS2 3=ultra)"
    exit 2
    ;;
esac
DURATION="${RE4_R36S_SECONDS:-180}"
case "$DURATION" in *[!0-9]*|"") echo "Invalid duration"; exit 2;; esac
if [ "$DURATION" -lt 15 ] || [ "$DURATION" -gt 600 ]; then
  echo "Duration must be 15..600"; exit 2
fi
CACHE="$CORE/cache/shader_binaries"
mkdir -p "$CACHE" || exit 2
LOG="$CORE/diagnostico_ps2_${NAME}_darkos.txt"
(
  echo "==== R36S PS2 INSPIRED LOW SPEC GRAPHICS ===="
  date
  uname -a
  free -m
  echo "PS2_PRESET=$PROFILE PS2_PRESET_NAME=$NAME"
  echo "PS2_LIGHTS_MAX=$LIGHTS SKIP_DOF=$NO_DOF SKIP_FOG=$NO_FOG FAR_Z=$FAR_Z"
  echo "NOTE=FAR_Z means optional conservative whole-object camera-space cutoff; may cause popping"
  echo "PS2_SHADER_CACHE_FILES_BEFORE=$(find "$CACHE" -type f -name '*.bin' | wc -l)"
  for entry in r36s_game_ps2 re4_coreproc libre4_main.so libre4_platform.so; do
    if [ ! -f "$CORE/$entry" ]; then echo "MISSING_FILE=$entry"; exit 127; fi
  done
  if [ ! -d "$CORE/modules" ] || [ ! -d "$DATA" ]; then
    echo "MISSING_GAME_DATA_OR_MODULES=1"; exit 127
  fi
  chmod +x "$CORE/r36s_game_ps2" "$CORE/re4_coreproc" 2>/dev/null || :
  export SDL_VIDEODRIVER=kmsdrm
  export SDL_VIDEO_EGL_DRIVER=libEGL.so
  export RE4_GX_BASEVERTEX=0
  export RE4_R36S_SWAP_MODE=5
  export RE4_R36S_SKIP_DOF="$NO_DOF"
  export RE4_R36S_LIGHTS_MAX="$LIGHTS"
  export RE4_R36S_SKIP_FOG="$NO_FOG"
  export RE4_R36S_FAR_Z="$FAR_Z"
  export RE4_R36S_SHADER_CACHE_DIR="$CACHE"
  export LD_LIBRARY_PATH="$CORE${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  cd "$CORE" || exit 2
  ./r36s_game_ps2 "$CORE" "$CORE/modules" "$DATA" "$DURATION"
  rc=$?
  echo "PS2_SHADER_CACHE_FILES_AFTER=$(find "$CACHE" -type f -name '*.bin' | wc -l)"
  du -sk "$CACHE"
  printf 'PS2_TEST_EXIT_CODE=%s\n' "$rc"
  exit "$rc"
) > "$LOG" 2>&1
rc=$?
printf 'CubeEvil4 PS2 profile %s exited with code %s\nLog: %s\n' "$NAME" "$rc" "$LOG"
sleep 3
exit "$rc"
