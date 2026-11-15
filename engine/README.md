# Codex Engine (alvo Mac)

Motor de áudio do Codex: carrega o DSP do Faust (`dsp/codex_delay.dsp`), abre áudio
full-duplex pelo CoreAudio e aceita controle do simulador web por WebSocket.

## Arquitetura (portátil)

```
src/params.h        parâmetros (fonte única: id, faixa, padrão)
src/hal.h           interfaces HAL: AudioBackend e ControlServer
src/dsp_codex.h     invólucro do DSP gerado pelo Faust
src/engine.h        núcleo portátil (sem CoreAudio/WebSocket) — roda igual no pedal
src/audio_miniaudio.h  backend de áudio Mac (miniaudio → CoreAudio)
src/ws_server.h     servidor WebSocket mínimo (só localhost)
src/main.cpp        CLI, modos e fiação
```

Trocar de alvo (GP-5, UNO Q) = reimplementar `AudioBackend`/`ControlServer`; o DSP e o
`engine.h` não mudam.

## Compilar

```
cd engine && cmake -B build -G Ninja && cmake --build build
```
Requer `faust`, `cmake`, `ninja`. O `codex_delay.hpp` é gerado do `.dsp` na compilação.

## Usar

```
./build/codex-engine --selftest           # testa o DSP sem hardware (impulso → ecos)
./build/codex-engine --list               # lista dispositivos de áudio
./build/codex-engine --in "GP-5" --out "Alto-falantes" --sr 44100 --port 7070
./build/codex-engine --no-audio           # só o controle (para testar o WebSocket)
```

Depois, rode o simulador (`sim/`) e abra no navegador: ele conecta em `ws://localhost:7070`.

> Cuidado com microfonia: se entrada e saída forem alto-falantes/microfone do próprio
> Mac, o som realimenta. Use fone, ou a GP-5 como entrada.
