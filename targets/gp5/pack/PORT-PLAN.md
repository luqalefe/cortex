# Plano de port — rodar o pack do GuitarPedal na GP-5

Design completo para fazer o core de áudio + efeitos do GuitarPedal rodarem na GP-5,
sobre a plataforma do OpenGP5. Tudo aqui foi verificado lendo o código dos dois projetos e
medindo com o compilador ARM; o que falta é a codificação iterativa (compilar e corrigir).

## Os dois limites reais da GP-5 (DSP Cortex-M4F)

Do build do OpenGP5 (`platform/dsp/build.py`):
- **Variáveis normais (.data+.bss): só 19.968 bytes (~19,5 KB).**
- **Bloco grande (`.large`, `GP5_LARGE`): 124.928 bytes (~122 KB).**
- Código (seção DSP 0x63): ~64 KB (menos o cabeçalho) — folgado (pack ~24 KB).
- Áudio: mono in / estéreo out, 44,1 kHz, blocos de 64, ~278.000 ciclos/bloco.

**Consequência:** o v1 tem ~81 KB de bss — **não cabe nos 19,5 KB** normais. Tem que ir para
o banco de 122 KB (`.large`). Essa é a modificação central do port.

## Como alimentar o core deles (verificado no bench)

O core expõe `single_sample(float mix)` que processa UMA amostra, lendo/escrevendo um anel
`i2s_dma_buf[16]` via dois "registradores" de DMA. O bench emula isso (`bench_sample`):
por amostra — escreve a entrada no slot, aponta rx 1 slot à frente e tx 8 atrás, avança um
timer fake, chama `single_sample(mix)`, lê a saída do slot. Replicamos isso em `fw_process`
para os 64 samples do bloco.

## Ordem de include (do bench.c), no nosso `dsp/firmware.h`

```
pico/stdlib.h, hardware/dma.h, hardware/timer.h      (shim do M4F)
status.h                                             (Firmware/)
Audio/types.h util.h envelope.h single-pole.h biquad.h fft.h analyze.h
#define effect_chain[], routed_effect_count          (nós definimos)
effect-state.h                                       (Firmware/)
Audio/effect.h                                        (o core: single_sample)
gen/effect_map.h                                      (gerado do subconjunto)
```

## Símbolos que o includer (nós) deve definir
- `uint8_t effect_chain[MAX_ROUTED_EFFECTS]; uint8_t routed_effect_count;`
- `sample_t get_usb_audio_input(void)` / `get_radio_audio_input(void)` → silêncio
- `void put_radio_audio_output(raw_sample_t, raw_sample_t)` → vazio

## Inicialização (fw_init, espelhando o main do bench)
1. `reset_effect(effects[i])` para todos.
2. `routing_start()` → `routing_add(&r, id)` para cada efeito da cadeia → `routing_end(r)`.
3. `set_effect_pot(...)` / `set_effect_mix(...)` com os valores vindos dos settings da GP-5.
4. laço `prepare()`/`init()` por efeito.
5. `init_meters()`.

## Shim do M4F (espelha `Validation/bench/shim/`, ~126 linhas)
- `pico/platform/sections.h`: `__not_in_flash*`/`__time_critical_func` → vazios.
- `hardware/dma.h`: struct com `read_addr/write_addr` + `dma_hw`.
- `hardware/timer.h`, `pico/stdlib.h`: mínimos.
- `Audio/cycles.h`: contador trivial (a medição real vem do OpenGP5, ver abaixo).

## A modificação de RAM (.large) — o passo central
Os efeitos declaram buffers como `static` comum → iriam para `.bss` (19,5 KB) e estouram.
Opções (em ordem de preferência):
1. **Linker:** um `link.ld` próprio (derivado do `platform/dsp/link.ld`) que joga o `.bss`/
   `.data` das unidades do pack para a região de 122 KB. Não toca no código dos efeitos.
2. Marcar os buffers grandes com `GP5_LARGE` (edição por efeito; mais invasivo).

## Build (truque de staging do OpenGP5)
O build do OpenGP5 só adiciona `-I<fw>/dsp`. Então o `build-pack.sh` monta uma pasta
`dsp/` com TUDO preservando subpastas (`Audio/…`, `Firmware/…`, `gen/…`, shim `pico/…`,
`hardware/…`) + nosso `firmware.h`. Aí `"Audio/x.h"` e `"x.h"` resolvem todos por
`-I<fw>/dsp`. Depois chama o build do DSP do OpenGP5 com o `link.ld` modificado.

## Medição de CPU (o que o usuário pediu)
A GP-5/OpenGP5 já mede ciclos por bloco e expõe:
- `GP5_CYCLES_LAST` (0x20007F74), `GP5_CYCLES_WORST` (0x20007F78), `GP5_LATE_BLOCKS` (0x20007F7C).
Plano: após rodar, ler `GP5_CYCLES_WORST` por USB (um comando de link do shim do OpenGP5 que
lê esse endereço) com a cadeia montada. Pior-caso/bloco ÷ 278.000 = % de carga. Repetir por
efeito/cadeia para a tabela de custo. **É medição no pedal — exige o firmware rodando.**

### Estimativa prévia (a confirmar no pedal)
Orçamento ~4.350 ciclos/amostra. Um biquad ~10–20 ciclos; um drive (ganho+clip+2 biquads)
~50–100; uma cadeia de vários efeitos leves cabe bem. Pesados (reverb 8 combs, FFT) custam
mais e podem apertar — por isso entram contados. Números reais virão do `GP5_CYCLES_WORST`.

## Estado
- [x] Receita de build do DSP (OpenGP5) entendida; limites de RAM/código medidos.
- [x] Forma de dirigir o core (anel i2s + single_sample) e init, extraídos do bench.
- [x] Subconjunto v1 que cabe em RAM (se realocado p/ `.large`): ~81 KB.
- [ ] Shim do M4F + `firmware.h` driver + `build-pack.sh` (staging).
- [ ] `link.ld` que realoca o bss do pack para o banco de 122 KB.
- [ ] Primeiro build + flash + leitura de `GP5_CYCLES_WORST` (CPU real).
