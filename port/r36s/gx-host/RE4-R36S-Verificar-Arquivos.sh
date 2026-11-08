#!/bin/sh
# Inspect an existing R36S CubeEvil4 installation without changing data/saves.
set -u
BASE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
ROOT="$BASE/cubeevil4_live_gx"
REPORT="$BASE/RELATORIO-R36S-ARQUIVOS.txt"
missing=0
{
  echo "==== CUBEEVIL4 R36S / DARKOSRE - VERIFICACAO DE ARQUIVOS ===="
  date
  echo "DIRETORIO=$ROOT"
  echo "This script only checks files, and does not restore proprietary game assets."
  for f in r36s_game_showcase_exitfix r36s_game_boot re4_coreproc libre4_main.so libre4_host.so libre4_platform.so; do
    if [ -s "$ROOT/$f" ]; then
      echo "OK: $f"
    else
      echo "FALTANDO: $f"
      missing=$((missing+1))
    fi
  done
  if [ -d "$ROOT/modules" ]; then
    set -- "$ROOT"/modules/libre4_*.so
    if [ -f "$1" ]; then
      echo "MODULOS_COMPILADOS=$#"
      if [ "$#" -lt 114 ]; then
        echo "FALTANDO_MODULOS=esperados_pelo_menos_114"
        missing=$((missing+1))
      fi
    else
      echo "FALTANDO: modules/libre4_*.so"
      missing=$((missing+1))
    fi
  else
    echo "FALTANDO: modules/"
    missing=$((missing+1))
  fi
  for f in data/disc0/sys/boot.bin data/disc0/sys/fst.bin; do
    if [ -s "$ROOT/$f" ];then echo "OK: $f";else
      echo "FALTANDO: $f (EXTRAIR DO SEU PROPRIO JOGO G4BE08)"
      missing=$((missing+1))
    fi
  done
  if [ -d "$ROOT/data/disc0/files" ];then
    echo "OK: data/disc0/files/ (folder exists; individual game assets not audited)"
  else
    echo "FALTANDO: data/disc0/files/ (EXTRAIR DO SEU PROPRIO JOGO)"
    missing=$((missing+1))
  fi
  if [ -s "$ROOT/data/disc0/sys/boot.bin" ];then
    discid=$(dd if="$ROOT/data/disc0/sys/boot.bin" bs=1 count=6 2>/dev/null)
    echo "GAME_ID=$discid"
    if [ "$discid" != "G4BE08" ];then echo "ATENCAO: game disc ID nao e o G4BE08 esperado";missing=$((missing+1));fi
  fi
  if [ -f "$ROOT/data/memcard/card-a.bin" ];then
    echo "MEMCARD=existe (NAO sobrescrever saves)"
  else
    echo "MEMCARD=nao encontrado; restaure seu backup se tiver progresso salvo"
  fi
  echo "TOTAL_PROBLEMAS=$missing"
  if [ "$missing" -gt 0 ];then
    echo "RESULTADO=INSTALACAO_INCOMPLETA"
    echo "Os arquivos de jogo (data/disc0) nao estao nos pacotes de compilacao."
    echo "Restaure a partir de uma extracao legal de sua propria copia do GameCube."
  else
    echo "RESULTADO=ARQUIVOS_ESSENCIAIS_ENCONTRADOS"
    echo "A presenca de arquivos nao comprova funcionamento de audio/controles/GPU."
  fi
} > "$REPORT"
cat "$REPORT"
echo "RELATORIO=$REPORT"
[ "$missing" -eq 0 ]
