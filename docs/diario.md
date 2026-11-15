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

## 2026-11-14 — Hipótese de hardware (informação externa, não verificada)

- Relato trazido pelo dono, de fonte pública não citada: o chip principal teria marcação customizada/obscura ("JAT S24PQ" ou parecida), junto com um chip **MVSilicon** (Bluetooth, áudio e interface). A Valeton não divulga o DSP.
- Avaliação: **hipótese**. "JAT S24PQ" não corresponde a nenhum fabricante conhecido; pode ser remarcação ou erro de leitura. MVSilicon é plausível (SoCs de áudio BT com núcleo e DSP), e pode ser ele o processador principal. A ligação SnapTone ↔ NAM não está confirmada.
- Como verificar: A9 (cpu_rec no arquivo de atualização) para o conjunto de instruções; B1/B2 (fotos próprias com marcação legível, contagem de pinos, encapsulamento).
- Risco anotado: SDKs da MVSilicon costumam ser distribuídos sob NDA. Se o firmware rodar nele, pode não haver compilador aberto. Reforça a arquitetura portátil (HAL).

## 2026-11-14 — A4 + A9 Firmware oficial

- Baixados do site oficial (valeton.net) os dois arquivos existentes: GP-5 Firmware V1.0.3 (lançamento inicial, 23/05/2025) e V1.0.6 (30/06/2025). Em `~/codex-private/firmware/V1.0.x/`. Tamanhos ~2,09 MB; SHA-256 no laudo. Notas de versão (PDF) extraídas com nomes simples (os originais tinham nome em chinês, o que causou o aviso "disk full?" no unzip — não era disco cheio; há 121 GB livres).
- Análise com `re/fw-analyze.py` + inspeção de estrutura. Resultados em `docs/gp5/firmware.md`:
  - Container próprio com magic ASCII **`HTFW`** (provável "HoTone FirmWare"), palavra de versão, tamanho e tabela de seções. É um pacote multi-seção, não imagem única.
  - **Não criptografado** (entropia 6,58; 4392 strings legíveis). Cauda de só 136 B → **provavelmente não assinado** (sem bloco de assinatura; SHA-256 embutido é para integridade). Compressão **parcial** (regiões isoladas de entropia alta = recursos/áudio; GIF de boot embutido).
  - Conjunto de instruções: cpu_rec dá ARMhf no início e **NDS32 (Andes)** na maior região de código. Strings mostram pilha BLE BTstack, USB device, SDIO e áudio → firmware do SoC principal (casa com MVSilicon BP1048B2, "RISC 32 bits"). Hipótese: núcleo **Andes NDS32**, toolchain menos comum. Confiança baixa/média.
  - Ambiguidade: não está claro se o JAT tem firmware próprio em memória separada. A confirmar na Fase B.
- Implicação para o Codex: substituição pela via de atualização parece plausível (sem cripto/assinatura forte); mas o alvo pode ser Andes, não ARM — reforça HAL + DSP portátil. Passo decisivo: ler IDCODE pela porta de depuração (J7).

## 2026-11-14 — Versão Mac do Codex (início)

- Criado `engine/` com arquitetura portátil (HAL): `engine.h` e o DSP não conhecem CoreAudio nem WebSocket; cada alvo implementa `AudioBackend`/`ControlServer`.
- DSP: `codex_delay.hpp` gerado do `.dsp` pelo Faust na compilação (via CMake). Invólucro em `dsp_codex.h` usando MapUI para ajustar parâmetros por nome.
- Áudio Mac: `audio_miniaudio.h` (miniaudio 0.11.21 vendido em `third_party/`, CoreAudio duplex, f32, de/entrelaçamento por canal).
- Controle: `ws_server.h`, servidor WebSocket mínimo escrito à mão (SHA-1 + base64 para o handshake, frames de texto mascarados, ping/pong, só localhost).
- Simulador `sim/`: interface do pedal em HTML/JS puro (4 knobs SVG arrastáveis, footswitch de bypass, LED de nível, OLED), conecta no motor por WebSocket com reconexão.
- Testado: `--selftest` detecta ecos do delay em 300/600/900 ms com realimentação decaindo (0,50→0,15→0,06) = DSP correto. `--list` enxerga a GP-5. Teste de ponta a ponta do WebSocket (cliente Python): handshake 101 + Sec-WebSocket-Accept corretos, `mix` e `bypass` aplicados e estado devolvido.
- Pendente: ouvir o áudio ao vivo (precisa de fone/interface para evitar microfonia); mais efeitos no DSP; presets.
