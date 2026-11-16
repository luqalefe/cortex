# Arquitetura do Cortex — um "SO" de efeitos portáteis

O objetivo do Cortex é ser uma base onde um **efeito é escrito uma vez** e roda em várias
plataformas (Mac, Valeton GP-5, Arduino UNO Q…), mantendo o mesmo comportamento. O usuário
(ou a comunidade) importa o efeito que quiser; cada plataforma é só um "driver" por baixo.

```
          ┌─────────────────────────────────────────────┐
          │              Efeito do Cortex                 │   effects/<nome>/
          │  C portátil: fw de áudio + tabela de params  │   (ex.: delay)
          │  codex_<efeito>_init / _process / _set       │
          └───────────────┬───────────────┬─────────────┘
                          │               │
             contrato portátil (effects/codex_effect.h)
                          │               │
        ┌─────────────────┘               └──────────────────┐
        ▼                                                      ▼
  Alvo Mac (engine/)                                   Alvo GP-5 (targets/gp5/)
  motor próprio + CoreAudio                            shim -> platform do OpenGP5
  controle por WebSocket                               áudio no Cortex-M4F, flash por USB
  precisão total, 2 s de delay                         int16, ~1 s de delay, controles físicos
```

## As camadas

### 1. Efeito (`effects/`)
C simples, sem dependência de plataforma. Um efeito define:
- `codex_<efeito>_init(sr)`, `_process(in, outL, outR, n)`, `_set_index(i, v)` (e `_set(nome, v)` no host);
- uma tabela `CortexParam` (id, nome, min, max, padrão).

`effects/codex_effect.h` isola as diferenças de plataforma com perfis:
- **host/Mac:** `float` em tudo, `<math.h>`, buffers normais, delay até 2 s;
- **GP-5** (`CODEX_TARGET_GP5`): matemática do platform do OpenGP5, buffers grandes em `int16`
  no bloco de 122 KB, delay até ~1 s. **As diferenças são explícitas, não escondidas** — a
  consistência é de comportamento, dentro dos limites honestos de cada alvo.

Primeiro efeito: `effects/delay/codex_delay.h` (delay com realimentação e tom). Mesmo
comportamento do DSP Faust do motor Mac (ecos em 300/600/900 ms, decaimento conferido).

### 2. Alvo Mac (`engine/`)
Motor em C++ com HAL próprio (CoreAudio via miniaudio, controle por WebSocket, simulador em
`sim/`). Hoje hospeda o DSP via Faust; vai passar a hospedar também os efeitos portáteis do
Cortex pela mesma API (a converter).

### 3. Alvo GP-5 (`targets/gp5/`) — backend: OpenGP5
O [OpenGP5](https://github.com/OpenGP5/OpenGP5) (MIT) é firmware aberto para a GP-5, com dois
processadores (controle Andes NDS32 + DSP ARM Cortex-M4F), flasher por USB e um platform
pronto. O Cortex o usa como **driver do alvo GP-5**: não reescrevemos flasher nem HAL.

- `targets/gp5/firmwares/<efeito>/dsp/firmware.h` — shim que converte o áudio int32 e os
  settings numerados do OpenGP5 para a API portátil do efeito.
- `targets/gp5/firmwares/<efeito>/nds32/firmware.c` — mapeia knobs/footswitch nos settings.
- `targets/gp5/build.sh` — faz o staging (efeito + headers lado a lado) e chama o build do
  OpenGP5, sem modificar o clone dele.

Licença: OpenGP5 é MIT, compatível com a GPLv2 do Cortex. Créditos preservados.

## Fluxo de um efeito novo

1. Escrever `effects/<efeito>/codex_<efeito>.h` (C portátil) e testar no PC.
2. Rodar no Mac pelo motor (áudio + simulador).
3. Para a GP-5: criar o shim em `targets/gp5/firmwares/<efeito>/`, `build.sh` e flashar pelo OpenGP5.

## Estado atual

- [x] Contrato portátil (`effects/codex_effect.h`) + delay portado (`effects/delay/`).
- [x] Delay validado no perfil host **e** no perfil GP-5 (via harness do OpenGP5, matemática do platform, int16).
- [x] Shim GP-5 do delay (DSP + controle) e `build.sh` de staging.
- [ ] Motor Mac hospedar o efeito portátil pela mesma API (hoje usa Faust).
- [ ] Build real do `.bin` da GP-5 (precisa dos compiladores do OpenGP5, ~190 MB) e flash `[HUMANO]`.
- [ ] Generalizar o staging para qualquer efeito (hoje copia o delay explicitamente).
```
