#!/bin/bash
# Codex -> GP-5: constrói um efeito do Codex como firmware do OpenGP5 (o backend do alvo GP-5).
#
# O OpenGP5 só procura headers em <firmware>/dsp e em platform/dsp, então aqui fazemos o
# "staging": montamos uma pasta de firmware completa (shim + headers portáteis do Codex) e
# chamamos o build do OpenGP5 sobre ela, sem modificar nada no clone do OpenGP5.
#
#   targets/gp5/build.sh codex-delay [out.bin]
#
# Requer o clone do OpenGP5 e os dois compiladores dele:
#   OPENGP5_DIR=~/dev/sdk/OpenGP5   (padrão)
#   cd "$OPENGP5_DIR" && release/setup-toolchain.sh     (uma vez, ~190 MB)
# Depois: python3 "$OPENGP5_DIR"/tools/gp5tool.py flash <out.bin> --sections all
set -euo pipefail

CODEX_ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OPENGP5_DIR=${OPENGP5_DIR:-$HOME/dev/sdk/OpenGP5}
NAME=${1:?uso: targets/gp5/build.sh <efeito> [out.bin]}
FW_SRC="$CODEX_ROOT/targets/gp5/firmwares/$NAME"
OUT=${2:-$CODEX_ROOT/build/GP-5_$NAME.bin}

[ -d "$OPENGP5_DIR" ] || { echo "OpenGP5 não encontrado em $OPENGP5_DIR (set OPENGP5_DIR)"; exit 1; }
[ -f "$FW_SRC/dsp/firmware.h" ] || { echo "sem $FW_SRC/dsp/firmware.h"; exit 1; }
[ -f "$OPENGP5_DIR/platform/build_firmware.sh" ] || { echo "clone do OpenGP5 incompleto"; exit 1; }

# Pasta de staging: shim + headers portáteis do Codex, lado a lado (include plano).
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/dsp" "$STAGE/nds32"
cp "$FW_SRC/dsp/firmware.h"              "$STAGE/dsp/"
cp "$CODEX_ROOT/effects/delay/codex_delay.h" "$STAGE/dsp/"   # TODO: por efeito; generalizar depois
cp "$CODEX_ROOT/effects/codex_effect.h" "$STAGE/dsp/"
cp "$FW_SRC/nds32/"*.c                   "$STAGE/nds32/"

echo "== staging em $STAGE (efeito: $NAME)"
mkdir -p "$(dirname "$OUT")"
( cd "$OPENGP5_DIR" && bash platform/build_firmware.sh "$STAGE" "$OUT" )
echo "== imagem: $OUT"
echo "   flash:  python3 $OPENGP5_DIR/tools/gp5tool.py flash \"$OUT\" --sections all"
