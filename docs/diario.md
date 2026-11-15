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

## 2026-11-15 — OpenGP5 como backend do alvo GP-5

- O usuário apontou o repositório **OpenGP5** (github.com/OpenGP5/OpenGP5, MIT): firmware aberto para a GP-5 que já resolve o que a nossa Fase A/B levantava — firmware dos dois processadores escrito do zero, flasher por USB (até web), emulador e framework de efeitos em C.
- Confirma e corrige a nossa RE: controle = **BP1048B2 / Andes NDS32 V3 (core D1088)**; o chip **JAT é o DSP de áudio, ARM Cortex-M4F** (~192 MHz), não Andes. Formato **HTFW**, SysEx com nibbles + CRC-8 poli 0x07, flash SPI 2 MB, J7 = SWD, link UART 460800 8N1, áudio mono-in/estéreo-out 44,1 kHz em blocos de 64. Tudo bate com o que tínhamos.
- Decisão do usuário: **clonar e aproveitar** (sala-limpa estrita suspensa para a parte GP-5; MIT é compatível com GPLv3). Visão: Codex como "SO" de efeitos portáteis (usuário importa efeitos nossos/da comunidade, rodam em várias plataformas). Modo de integração escolhido: **OpenGP5 = backend do alvo GP-5** (API de efeito própria e portátil; OpenGP5 como driver).
- Clonado em `~/dev/sdk/OpenGP5` (commit 2040b64), fora do repo.
- Criada a fundação da arquitetura (ver `docs/architecture.md`):
  - `effects/codex_effect.h` — contrato portátil com perfis (host: float/2 s; GP-5: int16/~1 s, matemática do platform).
  - `effects/delay/codex_delay.h` — delay portado para C puro (realimentação + tom), instância única.
  - `targets/gp5/firmwares/codex-delay/{dsp/firmware.h,nds32/firmware.c}` — shim que liga o efeito ao platform do OpenGP5 (áudio int32<->float, settings numerados; knob=mix, PARA=time, foot=bypass).
  - `targets/gp5/build.sh` — staging (efeito + headers lado a lado) + build do OpenGP5, sem tocar no clone.
- Testado: delay compila e roda nos **dois** perfis. Host (float): ecos 300/600/900 ms, 0,575→0,165→0,061. GP-5 (int16, via harness `run_effect.c` + `gp5_math.c` do OpenGP5): ACK nos settings, ecos 300/600/900 ms decaindo. Mesmo comportamento do DSP Faust do motor Mac.
- Limite honesto registrado: delay máximo cai de 2 s (Mac) para ~1 s na GP-5 (122 KB em int16). Nada foi gravado no pedal.
- Próximos: motor Mac hospedar o efeito portátil pela mesma API (hoje Faust); generalizar o staging p/ qualquer efeito; build real do .bin (precisa dos compiladores do OpenGP5, ~190 MB) e flash [HUMANO].

## 2026-11-15 — Diagnóstico do Bluetooth + renomeação p/ Cortex

- **Bluetooth da GP-5 diagnosticado: hardware OK, defeito é de software/app.** Com o BT ligado no pedal (ícone na tela), o Mac viu o pedal anunciando como `GP-5 BLE` (RSSI -59 dBm, forte), **conectou** por BLE e leu o GATT: expõe o **serviço BLE-MIDI padrão** (03B80E5A-…) com a característica de dados (write/read/notify). Logo, rádio + antena + pilha BLE + GATT funcionam. O "não funciona" é do app/celular (bate com relatos de incompatibilidade no Android). Ferramentas: `re/ble-scan.py`, `re/ble-connect.py` (bleak no venv).
- **Bônus:** como fala BLE-MIDI padrão, dá pra controlar o pedal sem o app da Valeton (canal extra p/ o protocolo A5, além da USB) e oferecer controle sem fio no futuro.
- **Renomeação Codex → Cortex** (decisão do usuário). Por ora só o nome visível: README, docs e títulos. Identificadores no código (`codex_*`, `CODEX_*`, `CortexDsp`) e paths seguem `codex` e migram depois. Entradas antigas do diário ficam como estavam (log histórico).
- **Decisão de rota:** flashar o Cortex na GP-5 **primeiro**; captura dos amps originais (A3) fica para depois (via reversão à Valeton quando quiser). Ciente de que cada flash é ~40s + risco de brick, e que o firmware de fábrica é recuperável (loader intacto + `.bin` oficial guardado).
- Em andamento: download dos compiladores do OpenGP5 (`setup-toolchain.sh`, ~190 MB) para gerar o `.bin` real e flashar.

## 2026-11-15 — PRIMEIRO FLASH: Cortex rodando na GP-5

- **Marco:** primeiro firmware Cortex gravado em hardware real. Imagem `build/GP-5_codex-delay.bin` (delay portátil + shim do backend OpenGP5), gravada com `gp5tool.py flash --sections all --yes`.
- Pré-checagem só-leitura (`re/usb-midi-probe.py`) desmentiu o falso "não responde" do status: pedal respondeu `12 30 -> 11 30 01 00` (app Valeton) e emite SysEx espontâneo `14 08 01` periódico (o que confundia o `gp5tool status`). Protocolo compatível.
- Flash: DSP 0x63 (49.992 B, 2632 pacotes) + app 0x62 (20.496 B, 1079 pacotes), ~45 s, sem erro. O chatter espontâneo não atrapalhou a gravação.
- Pós-flash: pedal enumera como `GP-5 Open FW`, `gp5tool status` = "open firmware". Build real usou os compiladores Andes NDS32 + ARM (OpenGP5 `setup-toolchain.sh`) e Docker (debian:bookworm-slim) para o lado de controle.
- Recuperação disponível: Foot+PARA -> modo update; voltar à Valeton com o `.bin` oficial V1.0.6. Roteiro em `docs/gp5/roteiro-flash.md`.
- Sem backup da unidade de fábrica (firmware de fábrica não é legível); rede de segurança = imagem oficial + loader intacto.
- Pendente: testar áudio ao vivo (delay) e os controles físicos (knob PARA = time, volume = mix, footswitch = bypass).

## 2026-11-15 — GuitarPedal (Torvalds) + mudança de licença p/ GPLv2

- O usuário apontou github.com/torvalds/GuitarPedal: pedal digital (RP2354A Cortex-M33 + codec TI) com DSP em software. Arquitetura quase idêntica à nossa: **efeitos como headers compartilhados** entre firmware, bench e web app; **17 efeitos** (Rat, Klon, Frenchie, Boost, Preamp, cabinet, reverb, echo, flanger, phaser, tremolo, vibrato, autowah, pitch, EQ paramétrico, compressor, dehum); biblioteca DSP (`Audio/`: biquad, LFO, envelope, 1-polo, FFT); **web app por USB/BLE-MIDI** (`WebMIDI/`). Formato de efeito elegante: metadados nos comentários (`// POT: "Distortion" LINEAR(0 1)=0.45`, `ENUM(...)`), fonte única p/ DSP + UI.
- **Licença: GPLv2-only** (padrão Torvalds; sem "or later" nem SPDX). Incompatível com GPLv3.
- **Decisão do usuário: mudar o Cortex para GPLv2.** Unifica os três: GuitarPedal (GPLv2-only), OpenGP5 (MIT→compatível) e nosso código. Criados `LICENSE` (texto oficial GPLv2) e `NOTICE.md` (créditos/licenças de terceiros). Docs atualizados (GPLv3→GPLv2).
- Clonado como referência em `~/dev/sdk/GuitarPedal` (fora do repo).
- Aproveitável (a portar p/ o contrato de efeito do Cortex, com shim + checagem de CPU/RAM na GP-5): drives (Rat/Klon/Frenchie/Boost/Preamp), cabinet, echo, modulação, EQ, compressor — leves, cabem no M4F; reverb grande/pitch/FFT a avaliar (M4F tem menos fôlego que o RP2350). Avaliar também adotar o formato de metadados de efeito deles em vez de/junto com nosso JSON.
- Pendente: começar a portar um drive (ex.: Rat ou Klon) para validar o pipeline efeito-de-terceiro → contrato Cortex → host/GP-5.

## 2026-11-15 — Pack de efeitos do GuitarPedal rodando no host (Mac)

- Objetivo do usuário: aproveitar TODOS os efeitos do GuitarPedal na software e na GP-5.
- Descoberta do contrato: os efeitos NÃO são autônomos — dependem de acessores de pot gerados por `scripts/gen_effects.py` (a partir dos `// POT:` nos comentários), da lib `Audio/` (biquad, LFO, envelope, db_to_level…) e de stubs de plataforma (`bench/shim/`: sections.h etc.). O `effect.h` define o contrato (struct effect com init/step/pots, mix law, fade). O **bench** deles já compila tudo num PC.
- **Prova no Mac:** `cd ~/dev/sdk/GuitarPedal/Validation && make bench` compilou (gcc→clang), gerou `effect_map.h` com os ~20 efeitos. Rodando float32 estéreo por stdin→stdout: Boost (ganho+clip), Rat (distorção), Tape Echo (eco), Reverb (cauda) todos processam corretamente. => **pack inteiro roda no host.**
- Caminho de port confirmado: reusar `gen_effects.py` + `Audio/` + stubs; o bench é o molde do backend host, e um shim equivalente serve a GP-5 (M4F) com SAMPLES_PER_SEC=44100 e blocos de 64.
- Pendente (rollout): (1) integrar o pack ao app do Mac (engine toca ao vivo + UI gerada dos metadados dos efeitos); (2) port p/ GP-5 (firmware DSP OpenGP5 incluindo effect_map+Audio+stubs M4F); (3) checar fôlego do M4F — drives/EQ/mod/compressor devem caber; reverb grande/pitch/FFT a avaliar (RP2350 deles tem mais RAM/CPU).
- Nota: efeitos deles usam sample rate 48k (bench) vs 44,1k da GP-5 — tempos/coeficientes recalculam pelo SAMPLES_PER_SEC, sem reescrever efeito.

## 2026-11-15 — Pack de efeitos na GP-5: medição de memória e subconjunto v1

- Pergunta: dá pra portar o pack pra um device menor (GP-5)? Sim, com ajuste de memória.
- Medição real (ARM Cortex-M4F, 44,1 kHz) via `targets/gp5/pack/measure-footprint.sh`:
  - **Código (.text) nunca foi o gargalo:** ~24 KB (cabe nos 64 KB da seção DSP).
  - **Gargalo é RAM:** pack completo (25 efeitos) = **~295 KB** de bss+data → estoura os 160 KB da GP-5.
  - Três efeitos concentram quase tudo: echo (`int16_t[32768]` ~64 KB), reverb (Freeverb ~80 KB), pitch (`float[4*4096]` ~64 KB).
- **Modificação (v1):** `targets/gp5/pack/effects-v1.txt` = 22 efeitos (todos os leves, sem echo/reverb/pitch). Resultado: **~85 KB** de RAM → cabe com folga (sobra ~75 KB p/ 1 pesado encolhido depois). Efeitos "estruturais" (ex.: `radio`) precisam ficar mesmo sem entrar na cadeia.
- Próximos ajustes documentados no README do pack: echo encurtado/no bloco de 122 KB, pitch com `-DDISCONT_SHIFT=11`, reverb int16/menos combs.
- Ainda falta o firmware DSP que roda o pack na GP-5 (espelhar o `bench/shim` deles p/ M4F). Esta etapa quantificou e definiu o subconjunto que cabe.

## 2026-11-15 — Firmware do pack na GP-5: receita de port + constraint de RAM

- Iniciada a montagem do firmware do pack. Lido o build do DSP do OpenGP5 (`platform/dsp/build.py`) e o bench do GuitarPedal.
- **Constraint crítico descoberto:** a GP-5 tem só **19,5 KB para variáveis normais** (.data+.bss); os 122 KB grandes são a região `.large` separada. O v1 tem ~81 KB de bss em `static` comum → NÃO cabe nos 19,5 KB. Modificação central do port: **realocar o bss do pack para o banco de 122 KB** via link.ld próprio (sem editar os efeitos).
- Receita de integração extraída e documentada em `targets/gp5/pack/PORT-PLAN.md`: como dirigir o core (anel i2s + `single_sample`), ordem de include, símbolos a definir (effect_chain, get_usb/radio_audio_input, put_radio_audio_output), init (reset/routing/pots/init/meters), shim do M4F (espelha `bench/shim`), truque de staging (tudo sob `dsp/` para o `-I<fw>/dsp` do OpenGP5), e medição de CPU via `GP5_CYCLES_WORST`.
- **CPU:** medição real sai dos contadores do OpenGP5 (`GP5_CYCLES_WORST` @0x20007F78) com o firmware rodando no pedal — ainda não dá pra medir sem o firmware. Estimativa prévia: efeitos leves cabem bem no orçamento (~4.350 ciclos/amostra); pesados (reverb/FFT) a confirmar.
- Próximo: codificar shim+driver+link.ld, primeiro build, flash, e ler a carga real.
