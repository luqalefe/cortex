# Valeton GP-5 — interfaces USB

Fonte: `re/usb-enum.py`, `system_profiler SPUSBDataType`, `ioreg -p IOUSB -l`, CoreAudio e CoreMIDI, com a GP-5 ligada ao Mac (macOS 27.2.1) em 2026-11-14. Só foram feitas leituras de descritores padrão.

## Dispositivo

| Campo | Valor |
|---|---|
| VID:PID | `0x84EF:0x0184` |
| Fabricante / Produto | Valeton / GP-5 |
| Número de série | 20240829 (parece uma data: 29/08/2024) |
| bcdUSB / bcdDevice | 1.10 / 1.00 |
| Velocidade | Full Speed (12 Mb/s) |
| Classe do dispositivo | 0xEF/0x02/0x01 (composto com IAD) |
| Configurações | 1 (4 interfaces, bMaxPower 0 mA) |

## Interfaces

| Intf | Alt | Classe | Subclasse | Endpoints | Interpretação |
|---|---|---|---|---|---|
| 0 | 0 | 0x01 Áudio | 0x01 AudioControl | — | Controle de áudio UAC1 padrão |
| 1 | 0/1 | 0x01 Áudio | 0x02 AudioStreaming | `0x05` OUT isócrono, 192 B/ms | Reprodução **Mac → GP-5**: PCM 2 canais, 16 bits, 44 100 Hz |
| 2 | 0/1 | 0x01 Áudio | 0x02 AudioStreaming | `0x84` IN isócrono, 192 B/ms | Gravação **GP-5 → Mac**: PCM 2 canais, 16 bits, 44 100 Hz |
| 3 | 0 | 0x01 Áudio | 0x03 MIDIStreaming | `0x03` OUT bulk 64 B, `0x82` IN bulk 64 B | **USB-MIDI 1.0 padrão** |

Não há interface própria do fabricante (classe 0xFF) nem HID.

## Sistema operacional

- **CoreAudio:** "GP-5 Audio", 2 entradas e 2 saídas, taxa atual de 44 100 Hz. O descritor só anuncia uma taxa (44 100 Hz), então qualquer outra taxa pedida pelo software é convertida pelo macOS. **Use 44,1 kHz em todo o caminho da GP-5.**
- **CoreMIDI:** uma fonte e um destino chamados "GP-5".

## Conclusão

- Áudio e MIDI são **classes padrão** (UAC1 + USB-MIDI) e funcionam sem driver no macOS.
- O editor oficial muito provavelmente conversa pela **interface 3 (USB-MIDI), por SysEx**. É a única interface com transporte bidirecional de dados, e o mesmo esquema foi documentado publicamente para a GP-200. Confiança: **provável**. A confirmação depende da captura (A5).
- Consequência para A5: como é MIDI padrão, a captura também pode ser feita **no próprio Mac**, monitorando o CoreMIDI (por exemplo, com um monitor MIDI que mostre SysEx), sem precisar de Windows nem USBPcap. Isso só vale se existir uma versão do editor para macOS.
