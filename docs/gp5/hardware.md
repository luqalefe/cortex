# Valeton GP-5 — hardware (rascunho)

Fonte: foto de desmontagem trazida pelo dono em 2026-11-14 (origem não confirmada; cópia em `~/codex-private/fotos/2026-11-14-placa-externa.png`). Tudo aqui deve ser **confirmado com fotos próprias (B1)**.

## Placas

| Placa | Serigrafia | Observação |
|---|---|---|
| Principal | `GP-5_MB_V01_241115` | revisão V01, data provável 15/11/2024; tem os chips JAT e BP1048B2 numa face e U29 na outra |
| Tela/controles | `GP-5_LCD_V02_250530`, `CC256124`, `2525` | revisão V02, data provável 30/05/2025; tela, footswitch, 2 encoders/knobs, LEDs, conector J4 (4 pinos); fica empilhada sobre a principal |

Fontes das fotos: `~/codex-private/fotos/2026-11-14-placa-externa.png` e `-2.png` (thegearforum.com, tópico "Valeton GP-5", p. 10; **não é a unidade do dono**).

## Chips

| Ref. | Marcação lida | Fabricante | Função provável | Encapsulamento | Confiança |
|---|---|---|---|---|---|
| U11 | `BP1048B2` / `BP154CUP` | MVSilicon | SoC de áudio Bluetooth (BT, possivelmente USB/interface) | QFP ~48 pinos | marcação: confirmada na foto; função: provável |
| (centro) | `JAT` / `1823` / `2512` / `S24PQ-000` | desconhecido (remarcado?) | processador principal de efeitos? | QFP ~64 pinos, cristal de 24 MHz ao lado | hipótese |
| U29 (face inferior da principal) | ilegível na foto | ? | candidato a codec de áudio ou chip analógico (perto dos jacks) | TSSOP ~16 pinos | hipótese |
| X2 + cristal 24 MHz | — | — | clocks independentes dos dois chips | — | provável |

- `2512` parece código de data (2025, semana 12). `S24PQ-000` parece código de cliente/firmware.
- **Nenhuma flash SPI de 8 pinos (SOIC-8/WSON-8) visível** em nenhuma das faces fotografadas da placa principal, nem na face superior da placa de tela. Falta a face inferior da placa de tela.

## Conectores e pontos de teste

| Ref. | Formato | Posição | Hipótese |
|---|---|---|---|
| J7 | 1×5 furos, pino 1 marcado (▲) | junto ao chip JAT | **SWD** (3V3/SWDIO/SWCLK/GND/RST) ou UART+BOOT; candidato principal a depuração/gravação |
| J14 | 2×2 | perto dos regulares/jack | teste/serial? |
| J4 | 1×4 furos, placa de tela | perto do footswitch | UART (3V3/TX/RX/GND)? |
| 2×5 pads | grade de 10 pads, borda esquerda da principal | — | interligação placa principal ↔ tela, ou cabeçalho de teste |
| TP8 | ponto de teste | face inferior da principal | a medir |

## Onde fica o firmware

**Não determinado.** Sem flash SPI visível, a hipótese é memória interna (JAT e/ou BP1048B2). Se confirmado, o backup da B4 deve ser feito por SWD (B5), não por CH341A. Lembrar da regra 5: se houver proteção de leitura, documentar e parar.

## Próximos passos

1. B1: fotos próprias da placa inteira, dos dois lados, com marcações legíveis.
2. B3: medir J7 com multímetro (GND, 3V3, nível de repouso de cada pino).
3. A9: cpu_rec no arquivo oficial de atualização para descobrir o conjunto de instruções.
