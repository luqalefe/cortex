#!/bin/bash
# Mede o footprint (código .text + RAM estática BSS) do pack de efeitos do GuitarPedal
# compilado para o DSP da GP-5 (ARM Cortex-M4F), para um subconjunto de efeitos.
#
# Uso:
#   targets/gp5/pack/measure-footprint.sh [arquivo-de-subconjunto]
#
# Sem argumento: mede o pack COMPLETO (baseline).
# Com arquivo: uma lista (um basename por linha, ex.: boost.h) dos efeitos a INCLUIR;
#   linhas começando com '#' são ignoradas.
#
# Precisa do clone do GuitarPedal e dos compiladores do OpenGP5:
#   GUITARPEDAL_DIR=~/dev/sdk/GuitarPedal   (padrão)
#   OPENGP5_DIR=~/dev/sdk/OpenGP5           (padrão; usa o arm-none-eabi do fw-build)
set -euo pipefail

GP=${GUITARPEDAL_DIR:-$HOME/dev/sdk/GuitarPedal}
OG=${OPENGP5_DIR:-$HOME/dev/sdk/OpenGP5}
ARMGCC="$OG/fw-build/arm/bin/arm-none-eabi-gcc"
ARMSIZE="$OG/fw-build/arm/bin/arm-none-eabi-size"
SUBSET="${1:-}"

[ -d "$GP" ] || { echo "GuitarPedal não encontrado em $GP"; exit 1; }
[ -x "$ARMGCC" ] || { echo "compilador ARM ausente ($ARMGCC); rode setup-toolchain.sh"; exit 1; }

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/Effects" "$TMP/gen"
# raiz-fake: o gerador espera Firmware/midi/midi.h ao lado da pasta Effects
ln -s "$GP/Firmware" "$TMP/Firmware"

# monta a pasta de efeitos (todos, ou só o subconjunto)
if [ -n "$SUBSET" ]; then
  while read -r line; do
    line="${line%%#*}"; line="$(echo "$line" | tr -d '[:space:]')"
    [ -z "$line" ] && continue
    cp "$GP/Effects/$line" "$TMP/Effects/" 2>/dev/null || echo "aviso: $line não achado" >&2
  done < "$SUBSET"
else
  cp "$GP/Effects/"*.h "$TMP/Effects/"
fi

# gera o mapa de efeitos + tabelas de matemática (mesma receita do bench deles)
python3 "$GP/scripts/gen_effects.py" "$TMP/Effects/" \
    "$TMP/gen/effect_map.h" "$TMP/gen/effects.js" "$TMP/gen/dummy_map.md"
python3 "$GP/scripts/pow2.py"        "$TMP/gen/pow2.h" 8
python3 "$GP/scripts/log2.py"        "$TMP/gen/log2.h" 8
python3 "$GP/scripts/quarter_sine.py" "$TMP/gen/quarter_sine.h" 8
[ -f "$TMP/Effects/rat.h" ] && python3 "$GP/scripts/rat_clamp.py" "$TMP/Effects/rat.h" -o "$TMP/gen/rat_clamp.h"

# compila o core+pack para o M4F (44,1 kHz), só objeto, otimizado p/ tamanho
"$ARMGCC" -Os -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb \
  -ffast-math -fsingle-precision-constant -DSAMPLES_PER_SEC=44100.0f \
  -I"$GP/Validation/bench/shim" -I"$TMP/gen" -I"$GP" -I"$GP/Firmware" -I"$GP/Audio" \
  -c "$GP/Validation/bench/bench.c" -o "$TMP/pack.o" 2>/dev/null

echo "== Footprint do pack para GP-5 (ARM Cortex-M4F, 44,1 kHz) =="
[ -n "$SUBSET" ] && echo "   subconjunto: $SUBSET" || echo "   pack COMPLETO (baseline)"
echo "   efeitos incluídos: $(ls "$TMP/Effects" | wc -l | tr -d ' ')"
"$ARMSIZE" "$TMP/pack.o"
echo
echo "Limites da GP-5: código (.text) <= 64 KB (seção DSP) · RAM (bss+data) <= 160 KB (32+128)."
