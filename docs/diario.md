# Diário — estudo da Valeton GP-5

## 2026-11-14 — Preparação

- Repositório: o existente em `~/code/codex` (não `~/dev/codex`). Criadas as pastas `re/`, `tools/`, `docs/gp5/` e o `re/README.md`.
- `.gitignore`: entraram `codex-private/`, `*.bin`, `*.hex`, `*.dfu`, `*.img`, `*.pcapng` e `re/.venv/`.
- Pasta privada `~/codex-private/` com `referencias/{di}`, `presets-gp5`, `capturas-usb`, `firmware`, `backup`, `fotos` e `uart`.
- Ferramentas (brew): libusb 1.0.30, Wireshark/tshark 4.6.9, binwalk 3.1.0, flashrom 1.8.0, sigrok-cli 0.7.2, picocom 2024-07, python@3.12.
- Venv `re/.venv` (Python 3.12): pyusb 1.3.1, sounddevice 0.5.6, soundfile 0.14.0, numpy 2.5.4, scipy 1.18.1, pyserial 3.5; depois também python-rtmidi e matplotlib.
- cpu_rec clonado em `~/dev/sdk/cpu_rec` (funciona; o corpus é carregado na primeira execução).

## 2026-11-14 — A1 Interfaces USB

- Comandos: `re/.venv/bin/python re/usb-enum.py`, `system_profiler SPUSBDataType`, `system_profiler SPAudioDataType`, `ioreg -p IOUSB -l -n GP-5`, listagem do CoreMIDI (script Swift) e leitura do descritor de configuração bruto (GET_DESCRIPTOR padrão, 271 bytes).
- Resultado: VID 0x84EF, PID 0x0184, Full Speed, dispositivo composto. As 4 interfaces são todas padrão: AudioControl, 2× AudioStreaming (estéreo, 16 bits, **só 44 100 Hz**) e MIDIStreaming. Não há classe 0xFF nem HID. O CoreAudio mostra "GP-5 Audio" com 2 entradas e 2 saídas a 44,1 kHz; o CoreMIDI tem uma fonte e um destino "GP-5".
- Documentado em `docs/gp5/usb.md`. **A1 aceita.**

## 2026-11-14 — A2 Reamp (parte de software)

- Criados `tools/test-signals.py` (impulso, varredura log 20 Hz–20 kHz e varredura inversa, com 2 s de silêncio) e `tools/codex-reamp.py` (playrec simultâneo, latência por correlação cruzada, `--repeat` com estatística, `--latency-comp`).
- Teste de reamp pelo USB: impulso a −12 dBFS saindo para "GP-5 Audio" e gravação de "GP-5 Audio", canais 1 e 2. **O impulso não voltou**: correlação normalizada de 0,006 e gravação com só ~−41 dBFS RMS de ruído da entrada de instrumento. Conclusão: na configuração atual, o áudio do USB **não entra no processamento**. Pendente: o humano conferir se existe opção de reamp no menu da GP-5. Caso contrário, usar o modo analógico.
- **A2 pendente:** a medição de latência com impulso × 5 depende da ligação analógica feita pelo humano (roteiro em `docs/gp5/roteiro-referencias.md`, passo 2).

## 2026-11-14 — Observação de MIDI (somente leitura)

- Escuta passiva com python-rtmidi: 1 SysEx espontâneo de 16 bytes em ~6 s, com conteúdo todo em nibbles. O *Identity Request* universal não teve resposta. Detalhes em `docs/gp5/protocolo.md`. Nenhum comando de escrita enviado.

## 2026-11-14 — Ferramentas das próximas tarefas

- `tools/reamp-batch.py` + `docs/gp5/roteiro-referencias.md` (A3).
- `docs/gp5/roteiro-captura.md` (A5): 18 capturas, sem salvar, atualizar ou restaurar.
- `re/usb-parse.py` (A6): dump, remontagem de SysEx USB-MIDI, diff byte a byte e detecção de cabeçalho/tamanho/contador/checksum (soma, XOR, CRC-8/16/32, inclusive em nibbles). Ainda não testado com captura real.
- `re/fw-analyze.py` (A9): binwalk, entropia por blocos (CSV + PNG), categorias de strings (só contagens no resumo), cabeçalho (tamanho, CRC-32, hashes, entropia do fim) e cpu_rec. Testado com arquivo sintético de 4 MB: achou o campo de tamanho e o CRC-32 (~2 min).

## Bloqueios / aguardando o humano

1. Conferir se a GP-5 tem opção de reamp pelo USB; senão, fazer a ligação analógica e os testes de latência (A2).
2. A3 inteira (DI, presets, gravação em lote, nuvem), com as versões do firmware e do editor.
3. A4: exportar presets/IRs/SnapTones e baixar o arquivo de atualização da mesma versão (e da mais recente) para `~/codex-private/firmware/`.
4. A5: fazer as capturas. Há editor oficial para macOS ou só para Windows?
5. A8 (`re/gp5-ctl.py`) depende do protocolo (A7). Nenhum comando de escrita será enviado antes disso e da sua aprovação.
