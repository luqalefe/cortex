# Cortex

Pedaleira de efeitos programável em que **cada efeito é escrito uma única vez** e roda em
várias plataformas com o mesmo comportamento: o Mac (motor próprio e simulador web), a
Valeton GP-5 (firmware aberto) e, mais adiante, o Arduino UNO Q.

Funciona como um pequeno "SO" de efeitos: o efeito é C portátil, e cada plataforma é
apenas um *driver* por baixo dele.

> Detalhes em [`docs/architecture.md`](docs/architecture.md). Diário de bordo em
> [`docs/diario.md`](docs/diario.md).

## Fase atual

**Fase 3: o Cortex roda em hardware real.** A integração com a Valeton GP-5 deu certo:

| Marco | Resultado |
|---|---|
| Primeiro flash (`GP-5_codex-delay.bin`) | DSP 49.992 B + controle 20.496 B gravados em ~45 s, sem erro. O pedal passa a se identificar como `GP-5 Open FW` |
| Cortex Pack (efeitos do GuitarPedal) na GP-5 | gravado em 28 s e rodando ao vivo |
| CPU medida no pedal (cadeia Signal Chain + Boost) | **~83.800 ciclos/bloco ≈ 30 %** do orçamento de 278 mil, **0 blocos atrasados** |
| Memória do firmware do pack | código ~36 KB de 64 · RAM normal ~5,7 KB de 19,5 · banco grande ~15 KB de 122 |

Ou seja: efeitos escritos para outro pedal (RP2354A) rodam em tempo real no DSP Cortex-M4F da
GP-5, sem dropout, e ainda sobra CPU e RAM para encadear mais efeitos.

Fases do projeto:

1. ✅ **Fundação**: contrato portátil de efeito, delay em C validado nos perfis host e GP-5.
2. ✅ **Alvo Mac**: motor C++ com CoreAudio e simulador web por WebSocket.
3. ✅ **Alvo GP-5**: build real, flash, Cortex Pack v1 rodando e CPU medida. *(fase atual: refinamento)*
4. ⏳ **Unificação**: motor Mac hospedando os efeitos portáteis (hoje usa Faust) e UI gerada dos metadados.
5. ⏳ **Arduino UNO Q** (Linux arm64, Faust/LV2/JACK).

## Arquitetura

### Visão geral

```
                ┌──────────────────────────────────────────────┐
                │               Efeito do Cortex                │  effects/<nome>/
                │  C portátil: áudio + tabela de parâmetros     │  (ou efeito do GuitarPedal)
                │  codex_<efeito>_init / _process / _set        │
                └───────────────┬──────────────────┬───────────┘
                                │                  │
                  contrato portátil: effects/codex_effect.h
                    (perfil host: float · perfil GP-5: int16)
                                │                  │
          ┌─────────────────────┘                  └──────────────────────┐
          ▼                                                                ▼
  Alvo Mac (engine/)                                       Alvo GP-5 (targets/gp5/)
  C++ · HAL próprio · CoreAudio (miniaudio)                shim → platform do OpenGP5
  controle por WebSocket ──► simulador web (sim/)          DSP Cortex-M4F + controle NDS32
  float, delay até 2 s                                     int16, delay até ~1 s, flash por USB
```

A regra principal: **o efeito não sabe em que plataforma está.** Tudo o que muda entre alvos
(origem da matemática, onde ficam os buffers grandes, precisão, limites de memória) fica no
contrato e nos shims, nunca no código do efeito.

### 1. Camada de efeitos: `effects/` e `dsp/`

- **`effects/codex_effect.h`** é o contrato portátil. Cada efeito expõe:
  - `init(sr)`: prepara o estado para uma taxa de amostragem;
  - `process(in, outL, outR, n)`: entrada mono, saída estéreo, em blocos de `n` amostras;
  - `set_index(i, v)` (e `set(nome, v)` no host): muda um parâmetro;
  - uma tabela `CodexParam` (id, nome, mín., máx., padrão), a mesma em todos os alvos.
- **Perfis de plataforma**, escolhidos em tempo de compilação:

  | | Host / Mac | GP-5 (`CODEX_TARGET_GP5`) |
  |---|---|---|
  | Matemática | `<math.h>` | `gp5_math.h` do OpenGP5 (o DSP não tem libc) |
  | Memória de delay | `float` | `int16_t` no bloco `GP5_LARGE` de 122 KB |
  | Delay máximo | 2 s | ~1 s |

  As diferenças são explícitas: o comportamento é o mesmo, cada alvo dentro dos próprios limites.
- **`effects/delay/codex_delay.h`**: primeiro efeito portátil (delay com realimentação e tom).
  Validado nos dois perfis com ecos em 300/600/900 ms decaindo igual.
- **`dsp/codex_delay.dsp`**: versão Faust do mesmo delay, usada hoje pelo motor Mac e como
  referência de comportamento.

### 2. Alvo Mac: `engine/` e `sim/`

Motor em C++ com a lógica de áudio separada do sistema operacional:

```
 interface de áudio ──► AudioBackend (miniaudio → CoreAudio) ──► Engine ──► DSP ──► saída
                                                                  ▲
 navegador (sim/) ◄──── WebSocket ws://localhost:7070 ────► ControlServer
```

| Arquivo | Papel |
|---|---|
| `src/params.h` | parâmetros (fonte única) |
| `src/hal.h` | interfaces `AudioBackend` e `ControlServer` |
| `src/engine.h` | núcleo portátil, sem CoreAudio nem rede |
| `src/dsp_codex.h` | invólucro do DSP gerado pelo Faust |
| `src/audio_miniaudio.h` | backend de áudio (miniaudio → CoreAudio) |
| `src/ws_server.h` | servidor WebSocket mínimo (só localhost) |
| `src/main.cpp` | CLI, modos (`--selftest`, ao vivo) e ligação dos módulos |

O **simulador** (`sim/`, Vite + three.js) desenha o chassi, os knobs com escala, o display com
ecos animados e um medidor de nível. Os pedais são descritos em JSON (`sim/public/pedals/`).

### 3. Alvo GP-5: `targets/gp5/`

#### O hardware

```
 entrada ──► codec ──► DSP "JAT" ARM Cortex-M4F (~192 MHz) ──► saída estéreo
 (mono)                 44,1 kHz, blocos de 64 amostras
                        código ≤ 64 KB · RAM 19,5 KB normal + 122 KB grande
                               ▲
                               │ UART 460800 8N1 (settings numerados)
                               │
 knobs / PARA / footswitch ──► controle BP1048B2 (Andes NDS32) ◄──► USB-MIDI (SysEx) / BLE-MIDI
                                                                     flash por USB
```

#### Como o Cortex se encaixa

O [OpenGP5](https://github.com/OpenGP5/OpenGP5) (MIT) é usado como driver: platform dos dois
processadores, HAL e flasher por USB. Nada disso é reescrito aqui; o `build.sh` monta um
staging e chama o build dele sem alterar o clone.

```
 targets/gp5/firmwares/<efeito>/
   nds32/firmware.c   knobs e footswitch ──► settings inteiros (id, valor) pela UART
   dsp/firmware.h     shim: áudio int32 ⇄ float, settings ⇄ set_index ──► effects/<efeito>
```

Mapeamento do `codex-delay`: knob de volume = mix, PARA = time (20 ms por clique),
footswitch = bypass. Repeats e tone ficam nos padrões.

#### Cortex Pack: `targets/gp5/pack/`

Leva os efeitos do [GuitarPedal](https://github.com/torvalds/GuitarPedal) (GPLv2) para a GP-5:

```
 core de áudio do GuitarPedal + efeitos (headers) + lib Audio/
          │
          ▼
 pack.c         driver: emula o anel i2s/DMA, chama single_sample, expõe API em C
 firmware.h     liga o driver ao contrato gp5_dsp.h do OpenGP5
 link-pack.ld   joga o .bss do pack no banco de 122 KB (sem editar os efeitos)
 build-pack.sh  staging + pack.o separado + link com o responder + imagem HTFW
```

O gargalo é a RAM, não o código:

| Config | Efeitos | Código | RAM | Cabe? |
|---|---|---|---|---|
| Pack completo | 25 | ~24 KB | ~295 KB | ❌ |
| **Cortex Pack v1** (`effects-v1.txt`) | 22 | ~22 KB | ~85 KB | ✅ |

Echo (~64 KB), reverb (~80 KB) e pitch (~64 KB) ficam de fora até terem buffers reduzidos.
A segunda barreira foram os 19,5 KB de RAM normal, resolvida pelo `link-pack.ld`, que move
os buffers para o banco grande. Plano completo em
[`targets/gp5/pack/PORT-PLAN.md`](targets/gp5/pack/PORT-PLAN.md).

### Apoio

| Pasta | Conteúdo |
|---|---|
| `tools/` | reamp (toca e grava alinhando a latência), lotes de reamp, sinais de teste |
| `re/` | engenharia reversa em sala limpa da GP-5 (USB, MIDI, BLE, análise de firmware); nenhum arquivo do fabricante |
| `docs/gp5/` | hardware, protocolo, USB, firmware e roteiros de captura e de flash |
| `plugin/` | protótipo JUCE (AU/VST3) |
| `hardware/` | KiCad e SPICE (hardware próprio, futuro) |
| `docker/` | imagem Linux arm64 (Faust, LV2, JACK) para o alvo UNO Q |

## Fluxo de um efeito novo

1. Escrever `effects/<efeito>/codex_<efeito>.h` em C portátil e testar no computador.
2. Ouvir no Mac pelo motor e pelo simulador.
3. Para a GP-5: criar o shim em `targets/gp5/firmwares/<efeito>/`, rodar `build.sh` e gravar
   pelo OpenGP5.

## Início rápido

**Mac** (requer `faust`, `cmake`, `ninja`, `node`):

```bash
cd engine && cmake -B build -G Ninja && cmake --build build
./build/codex-engine --selftest                     # testa o DSP sem hardware
./build/codex-engine --in "GP-5" --out "Alto-falantes" --port 7070

cd ../sim && npm install && npx vite                # abre o simulador no navegador
```

**GP-5** (requer o clone e os compiladores do OpenGP5; no macOS, também Docker):

```bash
export OPENGP5_DIR=~/dev/sdk/OpenGP5 GUITARPEDAL_DIR=~/dev/sdk/GuitarPedal
targets/gp5/build.sh codex-delay                    # → build/GP-5_codex-delay.bin
targets/gp5/pack/build-pack.sh                      # → build/GP-5_cortex-pack.bin
```

> ⚠️ **Gravar firmware na GP-5 pode inutilizar o pedal.** Faça backup e leia
> [`targets/gp5/README.md`](targets/gp5/README.md) e
> [`docs/gp5/roteiro-flash.md`](docs/gp5/roteiro-flash.md) antes. A gravação é sempre manual.
> Para voltar ao de fábrica: footswitch + PARA ao ligar (modo update) e gravar o `.bin` oficial.

## Estado

- [x] Contrato portátil e delay validados nos perfis host e GP-5
- [x] Motor Mac com CoreAudio e simulador web por WebSocket
- [x] Primeiro flash na GP-5 (delay do Cortex)
- [x] Cortex Pack v1 rodando na GP-5 (~30 % de CPU, 0 atrasos)
- [ ] Medir cadeias maiores e mapear knobs/footswitch nos efeitos do pack
- [ ] Investigar o pico isolado de CPU (pior bloco 91,8 %, provável init)
- [ ] Trazer echo, reverb e pitch de volta ao pack com buffers reduzidos
- [ ] Motor Mac hospedando os efeitos portáteis (hoje usa Faust)
- [ ] Alvo Arduino UNO Q

## Licença

GPLv2 (ver [`LICENSE`](LICENSE)), compatível com o OpenGP5 (MIT) e o GuitarPedal (GPLv2).
Créditos em [`NOTICE.md`](NOTICE.md). Nenhum código, binário ou dado da Valeton faz parte do
repositório.
