# Alvo GP-5 (backend: OpenGP5)

Leva um efeito do Cortex para a Valeton GP-5 usando o [OpenGP5](https://github.com/OpenGP5/OpenGP5)
(MIT) como driver: platform dos dois processadores, HAL e flasher por USB — nada disso é
reescrito aqui. Veja a visão geral em [`docs/architecture.md`](../../docs/architecture.md).

## O que tem aqui

```
targets/gp5/
  build.sh                      staging + build pelo OpenGP5 (não modifica o clone)
  firmwares/<efeito>/
    dsp/firmware.h              shim DSP: áudio int32 <-> efeito portátil (float); settings -> params
    nds32/firmware.c            controle: knobs/footswitch -> settings numerados
```

O shim é fino de propósito: o efeito de verdade vive em `effects/`, compartilhado com o Mac.

## Pré-requisitos (uma vez)

```bash
export OPENGP5_DIR=~/dev/sdk/OpenGP5          # onde o OpenGP5 foi clonado
cd "$OPENGP5_DIR" && release/setup-toolchain.sh   # os dois compiladores (~190 MB)
# macOS: precisa do Docker (o compilador do processador de controle é um binário Linux 32-bit)
pip install mido python-rtmidi                 # para o flasher
```

## Construir

```bash
targets/gp5/build.sh codex-delay              # -> build/GP-5_codex-delay.bin
```

## Gravar no pedal  `[HUMANO]`

> **Risco de brick é real.** Testado pelo OpenGP5 em uma única unidade; em alguns casos a
> recuperação exige um FT232H soldado nos pads de debug. Backup verificado antes de qualquer
> gravação; ler [Safety] e [Known limits] do OpenGP5. Isto é um passo manual, não automatizado.

```bash
python3 "$OPENGP5_DIR"/tools/gp5tool.py status
python3 "$OPENGP5_DIR"/tools/gp5tool.py flash build/GP-5_codex-delay.bin --sections all
```

Voltar ao firmware de fábrica: flashar o `.bin` oficial da Valeton com o mesmo `gp5tool.py`.
Recuperação: desligar, segurar footswitch + knob PARA, religar (entra em modo update), flashar de novo.

## Testar sem o pedal (recomendado antes de flashar)

O `run_effect.py` do OpenGP5 roda o código de áudio num WAV no computador:

```bash
cd "$OPENGP5_DIR"
cc -O2 -std=c99 -Wall -fno-builtin \
   -I ~/code/codex/targets/gp5/firmwares/codex-delay/dsp \
   -I ~/code/codex/effects/delay -I ~/code/codex/effects -I platform/dsp \
   platform/host/run_effect.c platform/dsp/gp5_math.c -o /tmp/gp5_delay
# (ou, mais simples, use o build.sh e teste com o pedal)
```

## Mapeamento de controles (codex-delay)

| controle físico | setting | id |
|---|---|---|
| knob de volume | mix (0..1) | 2 |
| PARA girando | time (ms, 20 ms/clique) | 0 |
| footswitch | bypass (corta o molhado) | 2 |

`repeats` e `tone` ficam nos padrões (enviados uma vez quando o DSP acorda). Settings são
inteiros no fio; o shim DSP converte (ver `dsp/firmware.h`).

[Safety]: https://github.com/OpenGP5/OpenGP5#safety
[Known limits]: https://github.com/OpenGP5/OpenGP5/blob/main/GETTING-STARTED.md#known-limits
