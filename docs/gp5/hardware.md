# Valeton GP-5 — hardware (rascunho)

Fonte: foto de desmontagem trazida pelo dono em 2026-11-14 (origem não confirmada; cópia em `~/codex-private/fotos/2026-11-14-placa-externa.png`). Tudo aqui deve ser **confirmado com fotos próprias (B1)**.

## Chips

| Ref. | Marcação lida | Fabricante | Função provável | Encapsulamento | Confiança |
|---|---|---|---|---|---|
| U11 | `BP1048B2` / `BP154CUP` | MVSilicon | SoC de áudio Bluetooth (BT, possivelmente USB/interface) | QFP ~48 pinos | marcação: confirmada na foto; função: provável |
| (centro) | `JAT` / `1823` / `2512` / `S24PQ-000` | desconhecido (remarcado?) | processador principal de efeitos? | QFP ~64 pinos, cristal de 24 MHz ao lado | hipótese |
| X2 + cristal 24 MHz | — | — | clocks independentes dos dois chips | — | provável |

- `2512` parece código de data (2025, semana 12). `S24PQ-000` parece código de cliente/firmware.
- Não se vê flash SPI de 8 pinos nas fotos. Pendente: fotografar a placa inteira, dos dois lados.

## Conectores e pontos de teste

| Ref. | Formato | Posição | Hipótese |
|---|---|---|---|
| J7 | 1×5 furos, pino 1 marcado (▲) | junto ao chip JAT | **SWD** (3V3/SWDIO/SWCLK/GND/RST) ou UART+BOOT; candidato principal a depuração/gravação |
| J14 | 2×2 | perto dos regulares/jack | teste/serial? |
| J4 | furos na borda inferior | perto do footswitch | teste/programação? |

## Onde fica o firmware

**Não determinado.** Sem flash SPI visível, a hipótese é memória interna (JAT e/ou BP1048B2). Se confirmado, o backup da B4 deve ser feito por SWD (B5), não por CH341A. Lembrar da regra 5: se houver proteção de leitura, documentar e parar.

## Próximos passos

1. B1: fotos próprias da placa inteira, dos dois lados, com marcações legíveis.
2. B3: medir J7 com multímetro (GND, 3V3, nível de repouso de cada pino).
3. A9: cpu_rec no arquivo oficial de atualização para descobrir o conjunto de instruções.
