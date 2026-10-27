#!/bin/sh
# R36S DarkOSRE only: measure physical START, SELECT, A/B mappings outside
# the game. Doesn't modify, read or require Resident Evil assets/saves.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 2
ROOT="$HERE/cubeevil4_live_gx"
BIN="$ROOT/r36s_showcase_input_probe"
if [ ! -x "$BIN" ];then
  echo "A ferramenta de diagnostico nao esta instalada: $BIN"
  sleep 5
  exit 127
fi
mkdir -p "$ROOT/logs" || exit 2
LOG="$ROOT/logs/diagnostico_botoes_fisicos_r36s.txt"
echo "Durante 12 segundos pressione SELECT, START, A e B uma vez cada."
sleep 2
(
  echo "==== R36S LINUX + SDL CONTROLES FISICOS ===="
  date
  uname -a
  "$BIN" 12
  rc=$?
  echo "PROBE_EXIT_CODE=$rc"
  exit "$rc"
) > "$LOG" 2>&1
rc=$?
echo "Diagnostico finalizado. Relatorio:"
echo "$LOG"
sleep 5
exit "$rc"
