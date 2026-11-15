# Cortex Pack — efeitos do GuitarPedal na GP-5 (dispositivo menor)

Leva o pack de efeitos do [GuitarPedal](https://github.com/torvalds/GuitarPedal) (GPLv2)
para a Valeton GP-5, que é um alvo **menor** que o pedal original dele (RP2354A). Este
diretório trata do que precisa ser **ajustado de desempenho/memória** para caber.

## O limite real: RAM, não código

Medido com o compilador ARM (Cortex-M4F, 44,1 kHz) via `measure-footprint.sh`:

| Config | Efeitos | Código (.text) | RAM (bss+data) | Cabe na GP-5? |
|---|---|---|---|---|
| Pack completo | 25 | ~24 KB | **~295 KB** | ❌ RAM estoura (160 KB) |
| **Cortex Pack v1** | 22 | ~22 KB | **~85 KB** | ✅ com folga |

Limites da GP-5 (DSP Cortex-M4F): **código ≤ 64 KB** (seção de firmware do DSP) e
**RAM ≤ 160 KB** (32 KB rápidos + 128 KB). O **código nunca foi o gargalo** (~22 KB);
o problema é a **RAM dos buffers grandes**.

## De onde vinham os ~295 KB

Três efeitos concentram quase toda a memória:

| Efeito | Buffer | RAM |
|---|---|---|
| `echo` (delay) | `int16_t[32768]` (~0,74 s) | ~64 KB |
| `reverb` | 8 combs de 2048 + allpass (Freeverb) | ~80 KB |
| `pitch` | `float[4*4096]` | ~64 KB |

Os outros ~19 efeitos (drives, EQ, compressor, modulação, cabinet, tom…) usam poucos KB
cada.

## A modificação do v1

`effects-v1.txt` lista o subconjunto que vai para o pedal: **todos os leves**, deixando de
fora os 3 devoradores. Isso derruba a RAM de ~295 KB para **~85 KB** — cabe com folga, e
ainda sobra espaço para **um** efeito pesado depois, com buffer ajustado (ver abaixo).

Observação: alguns efeitos são **estruturais** (o core de áudio os referencia, ex.: `radio`)
e não podem sair do subconjunto, mesmo que não entrem na cadeia.

## Próximos ajustes (trazer um pesado de volta)

Com ~75 KB livres, dá para incluir **um** pesado por firmware, encolhido:

- **echo:** reduzir o array (ex.: `16384` = ~0,37 s → ~32 KB) ou alocá-lo no bloco de 122 KB
  (`GP5_LARGE`) do nosso shim da GP-5.
- **pitch:** `-DDISCONT_SHIFT=11` corta a tabela pela metade (~32 KB).
- **reverb:** os combs têm afinação fixa (Freeverb); encolher muda o som — alternativa é
  armazenar em `int16` ou usar menos combs (é uma edição do efeito, não um `-D`).

## Reproduzir a medição

```bash
targets/gp5/pack/measure-footprint.sh                        # pack completo (baseline)
targets/gp5/pack/measure-footprint.sh targets/gp5/pack/effects-v1.txt   # subconjunto v1
```
Precisa do clone do GuitarPedal (`GUITARPEDAL_DIR`, padrão `~/dev/sdk/GuitarPedal`) e dos
compiladores do OpenGP5 (`OPENGP5_DIR`, padrão `~/dev/sdk/OpenGP5`).

## Ainda falta (o firmware em si)

Isto quantifica e define o subconjunto que cabe. O **firmware DSP** que roda o pack na GP-5
(espelhando o `bench/shim` deles para o M4F, 44,1 kHz, blocos de 64, e mapeando
footswitch/knobs nos pots) é o próximo passo.
