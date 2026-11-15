#!/bin/bash
# Constrói o firmware "Cortex Pack" da GP-5: core de áudio + efeitos do GuitarPedal
# rodando sobre a plataforma do OpenGP5. Ver targets/gp5/pack/PORT-PLAN.md.
#
#   targets/gp5/pack/build-pack.sh [subconjunto.txt] [out.bin]
#
# Precisa dos clones e compiladores:
#   GUITARPEDAL_DIR=~/dev/sdk/GuitarPedal   OPENGP5_DIR=~/dev/sdk/OpenGP5
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
CODEX=$(cd "$HERE/../../.." && pwd)
GP=${GUITARPEDAL_DIR:-$HOME/dev/sdk/GuitarPedal}
OG=${OPENGP5_DIR:-$HOME/dev/sdk/OpenGP5}
SUBSET="${1:-$HERE/effects-v1.txt}"
OUT="${2:-$CODEX/build/GP-5_cortex-pack.bin}"

GCC="$OG/fw-build/arm/bin/arm-none-eabi-gcc"
OBJCOPY="$OG/fw-build/arm/bin/arm-none-eabi-objcopy"
OBJDUMP="$OG/fw-build/arm/bin/arm-none-eabi-objdump"
SIZE="$OG/fw-build/arm/bin/arm-none-eabi-size"
DSP="$OG/platform/dsp"

for p in "$GP" "$OG" "$GCC" "$DSP/responder.c"; do
  [ -e "$p" ] || { echo "faltando: $p"; exit 1; }
done

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
S="$TMP/stage"            # tudo sob -I$S (as inclusões usam "Audio/x.h", "status.h", etc.)
mkdir -p "$S" "$S/Effects" "$S/Audio"

# 1) efeitos do subconjunto + geração do mapa/tabelas
ln -s "$GP/Firmware" "$TMP/Firmware"   # gen_effects espera Firmware/midi ao lado de Effects
mkdir -p "$TMP/Effects"
while read -r line; do
  line="${line%%#*}"; line="$(echo "$line" | tr -d '[:space:]')"
  [ -z "$line" ] && continue
  cp "$GP/Effects/$line" "$TMP/Effects/" 2>/dev/null || echo "aviso: $line não achado" >&2
done < "$SUBSET"
cp "$TMP/Effects/"*.h "$S/Effects/"
python3 "$GP/scripts/gen_effects.py" "$TMP/Effects/" "$S/effect_map.h" "$S/effects.js" "$S/dummy_map.md"
for t in pow2 log2 quarter_sine; do python3 "$GP/scripts/$t.py" "$S/$t.h" 8; done
[ -f "$S/Effects/rat.h" ] && python3 "$GP/scripts/rat_clamp.py" "$S/Effects/rat.h" -o "$S/rat_clamp.h"

# 2) headers do core + plataforma-stub (preservando subpastas p/ "Audio/x.h")
cp "$GP/Audio/"*.h "$S/Audio/"
cp "$GP/Firmware/status.h" "$GP/Firmware/effect-state.h" "$S/"
cp -R "$GP/Validation/bench/shim/." "$S/"      # pico/, hardware/, Audio/cycles.h (sobrepõe)
cp "$HERE/src/firmware.h" "$S/firmware.h"      # usado pelo responder.c (-I$S)

# 3) compila o pack (TU separada) para o M4F
CF="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O2 -ffreestanding
    -fno-math-errno -nostdlib -ffunction-sections -fdata-sections -Wall"
# shellcheck disable=SC2086
"$GCC" $CF -DSAMPLES_PER_SEC=44100.0f -I"$S" -c "$HERE/src/pack.c" -o "$TMP/pack.o"

# 4) compila+linka o responder do OpenGP5 com nosso firmware.h, o pack.o e o linker custom
# shellcheck disable=SC2086
"$GCC" $CF -DGP5_FIRMWARE -DAUDIO_LEVEL -DAUDIO_PASSTHROUGH -DRESIDENT_TICK_PATCH \
  -I"$S" -I"$DSP" \
  -Wl,-T,"$HERE/src/link-pack.ld" -Wl,--gc-sections \
  "$DSP/responder.c" "$TMP/pack.o" -lgcc -o "$TMP/pack.elf"

echo "== seções (nome / tamanho hex / endereço) =="
"$OBJDUMP" -h "$TMP/pack.elf" | awk '/\.text|\.data|\.bss|\.large/{printf "  %-10s size 0x%s  vma 0x%s\n",$2,$3,$4}'
echo "== resumo (bytes decimais) =="
"$SIZE" "$TMP/pack.elf"

# 5) empacota na seção 0x63 do HTFW (igual ao build.py do OpenGP5)
"$OBJCOPY" -O binary --only-section=.text --only-section=.data "$TMP/pack.elf" "$TMP/pack.bin"
mkdir -p "$(dirname "$OUT")"
python3 - "$TMP/pack.bin" "$OUT" "$OG" <<'PY'
import sys, struct
binf, out, og = sys.argv[1], sys.argv[2], sys.argv[3]
sys.path.insert(0, og + "/tools")
import gp5tool
SEC_LEN, HDR_LEN, VTOR = 0xC348, 0x400, 0x4400
img = open(binf, "rb").read()
sp, reset = struct.unpack_from("<2I", img, 0)
assert 0x20000000 < sp <= 0x20008000 and reset & 1, (hex(sp), hex(reset))
hdr = b"\xff"*8 + b"V106" + struct.pack("<I", VTOR) + bytes(HDR_LEN-16)
assert HDR_LEN + len(img) <= SEC_LEN, f"código grande demais: {len(img)} > {SEC_LEN-HDR_LEN}"
new = hdr + img + b"\xff"*(SEC_LEN-HDR_LEN-len(img))
open(out, "wb").write(gp5tool.make_image({0x63: new}))
print(f"== imagem: {out}  (código {len(img)} bytes @0x{VTOR:x})")
PY
echo "   flash: re/.venv/bin/python $OG/tools/gp5tool.py flash \"$OUT\" --sections all --yes"
