# Roteiro — captura do protocolo do editor (A5) [HUMANO]

A GP-5 só tem interfaces padrão (áudio UAC1 + USB-MIDI; ver `usb.md`). O editor oficial muito provavelmente conversa por **SysEx na interface MIDI**. Daí dois caminhos:

## Caminho 1 — Windows + Wireshark + USBPcap (o mais completo)

1. Num PC com Windows (ou numa VM UTM/Parallels com a GP-5 repassada para ela), instale o Wireshark marcando a opção **USBPcap** e o editor oficial da Valeton.
2. Ligue a GP-5. Para descobrir o hub dela, abra o Wireshark: cada "USBPcapN" é um hub. Escolha o que mostra tráfego ao mexer num knob.
3. Para **cada item** da lista abaixo: inicie a captura, faça **só aquela ação**, espere 2 s, pare e salve com o nome indicado.

## Caminho 2 — Mac, se houver editor para macOS (alternativa)

O tráfego USB do Mac é difícil de capturar, mas como o transporte é MIDI padrão, um monitor de MIDI que mostre SysEx enxerga o que o **aparelho envia**. Ele não vê o que o editor envia. Use o caminho 2 só como complemento.

## Lista de capturas (uma ação por arquivo)

Salve em `~/codex-private/capturas-usb/`:

| # | Arquivo | Ação |
|---|---|---|
| 00 | `00-ocioso-30s.pcapng` | Editor fechado, GP-5 parada, 30 s (tráfego espontâneo) |
| 01 | `01-conectar-editor.pcapng` | Abrir o editor e esperar ele terminar de carregar |
| 02 | `02-ler-preset-01.pcapng` | No editor, selecionar o preset 01 |
| 03 | `03-volume-patch-50-para-51.pcapng` | Volume do patch: de 50 para 51 (uma unidade) |
| 04 | `04-volume-patch-51-para-52.pcapng` | Volume do patch: de 51 para 52 |
| 05 | `05-volume-patch-52-para-51.pcapng` | Volume do patch: de 52 para 51 |
| 06 | `06-trocar-preset-01-para-02.pcapng` | Trocar do preset 01 para o 02 pelo editor |
| 07 | `07-trocar-preset-02-para-01-no-aparelho.pcapng` | Trocar de preset no **aparelho** (footswitch) |
| 08 | `08-ligar-bloco-delay.pcapng` | Ligar o bloco de delay |
| 09 | `09-desligar-bloco-delay.pcapng` | Desligar o mesmo bloco |
| 10 | `10-delay-time-mais-1.pcapng` | Tempo do delay: +1 unidade |
| 11 | `11-delay-time-mais-1-de-novo.pcapng` | Tempo do delay: +1 unidade de novo |
| 12 | `12-mudar-tipo-de-amp.pcapng` | Trocar o modelo de amp para o próximo da lista |
| 13 | `13-mudar-tipo-de-amp-de-volta.pcapng` | Voltar ao modelo anterior |
| 14 | `14-gain-amp-mais-1.pcapng` | Gain do amp: +1 unidade |
| 15 | `15-knob-no-aparelho.pcapng` | Girar um knob **no aparelho** com o editor aberto |
| 16 | `16-afinador-ligar.pcapng` | Ligar o afinador (se o editor tiver) |
| 17 | `17-fechar-editor.pcapng` | Fechar o editor |

Anote no diário os valores de antes e depois de cada mudança, quando o editor mostrar números.

## Não fazer (proibido nesta task)

- **Salvar** preset na memória (botão Save/Write/Store). Se o editor salvar automaticamente, pare e me avise antes de continuar.
- Atualizar firmware.
- Restaurar o padrão de fábrica.
- Importar ou gravar IR, preset ou captura no aparelho.

Ao desligar e ligar a GP-5 depois das capturas, os presets devem voltar ao valor salvo. Confirme isso e anote no diário.
