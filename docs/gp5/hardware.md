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
| U29 (face inferior da principal) | `CM053B` (logo TI, lote `49K`/`ALE`) | Texas Instruments | **CD4053B (PW, TSSOP-16)**: chave analógica CMOS 3× SPDT, para roteamento de sinal (ex.: seleção de entrada/saída, mudo) — **não é codec** | TSSOP-16 | provável (`CM053B` é a marcação de topo da TI para o CD4053BPW) |
| X2 + cristal 24 MHz | — | — | clocks independentes dos dois chips | — | provável |

- `2512` parece código de data (2025, semana 12). `S24PQ-000` parece código de cliente/firmware.
- **Nenhuma flash SPI de 8 pinos (SOIC-8/WSON-8) visível** em nenhuma das faces fotografadas da placa principal, nem na face superior da placa de tela. Falta a face inferior da placa de tela.

**Codec de áudio:** nenhum codec dedicado visível nas faces fotografadas. Hipótese: as conversões A/D e D/A são feitas pelo **ADC/DAC integrado do BP1048B2** (SoCs de áudio da MVSilicon costumam integrar codec), com o JAT processando os efeitos. A confirmar pelo datasheet do BP1048B2 e pelas trilhas entre os jacks e os chips.

## Conectores e pontos de teste

| Ref. | Formato | Posição | Hipótese |
|---|---|---|---|
| J7 | 1×5 furos, pino 1 marcado (▲) | junto ao chip JAT | **SWD** (3V3/SWDIO/SWCLK/GND/RST) ou UART+BOOT; candidato principal a depuração/gravação |
| J14 | 2×2 | perto dos regulares/jack | teste/serial? |
| J4 | 1×4 furos, placa de tela | perto do footswitch | UART (3V3/TX/RX/GND)? |
| 2×5 pads | grade de 10 pads, borda esquerda da principal | — | **conector placa principal ↔ tela** (vista lateral mostra um conector fêmea 2×5 preto entre as placas) — provável |
| TP8 | ponto de teste | face inferior da principal | a medir |

## Vista lateral (foto 3)

- Conector fêmea 2×5 entre as placas: confirma a ligação placa principal ↔ tela por 10 vias.
- Indutor SMD grande perto da borda: provável conversor DC-DC (fonte/bateria).
- Bloco vermelho entre as placas: corpo do footswitch ou suporte; a confirmar.
- Encoder/potenciômetro com eixo, e jacks na lateral da placa principal.

## Onde fica o firmware

**Não determinado.** Sem flash SPI visível, a hipótese é memória interna (JAT e/ou BP1048B2). Se confirmado, o backup da B4 deve ser feito por SWD (B5), não por CH341A. Lembrar da regra 5: se houver proteção de leitura, documentar e parar.

## Próximos passos

1. B1: fotos próprias da placa inteira, dos dois lados, com marcações legíveis.
2. B3: medir J7 com multímetro (GND, 3V3, nível de repouso de cada pino).
3. A9: cpu_rec no arquivo oficial de atualização para descobrir o conjunto de instruções.
