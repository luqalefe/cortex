# Valeton GP-5 — laudo do arquivo oficial de atualização (A9)

Fonte: baixado do site oficial da Valeton em 2026-11-14. Dois arquivos, guardados em `~/codex-private/firmware/` (fora do repositório). Análise por `re/fw-analyze.py` e inspeção de estrutura. **Nenhum byte do firmware é reproduzido aqui**; só formato, offsets e natureza do conteúdo.

## Arquivos

| Versão | Arquivo (.bin) | Tamanho | SHA-256 (início) | Lançamento |
|---|---|---|---|---|
| V1.0.3 | `GP-5 Firmware V1.0.3.bin` | 2 090 508 B | `0fa42b1b…` | 23/05/2025 (primeira versão) |
| V1.0.6 | `GP-5 Firmware V1.0.6.bin` | 2 094 404 B | `259ac4e7…` | 30/06/2025 |

As notas de versão (PDF no zip) dizem: V1.0.3 é o lançamento inicial; V1.0.6 melhora o afinador em notas graves e corrige bugs. As duas versões são quase idênticas em estrutura (diferença de ~4 KB e pequenos deslocamentos), o que confirma que são a mesma plataforma.

## Container

- **Assinatura mágica:** os 4 primeiros bytes são o texto ASCII **`HTFW`** (provável "HoTone FirmWare" — a Hotone é a marca-mãe da Valeton). Confiança: provável.
- `@0x04`: palavra de versão. `@0x08`: tamanho total do arquivo (bate exatamente). Depois, uma **tabela de offsets e tamanhos de seções** (vários pares apontando para dentro do arquivo, por exemplo `@0x20`=0x50000, `@0x24`=fim dos dados, `@0x7c`=0x190000). Ou seja, o `.bin` é um **pacote com várias seções** (código, recursos, dados), não uma imagem única.
- **Cauda:** depois do fim dos dados (`@0x24`) sobram **136 bytes** de entropia ~6,5. É pequeno demais para uma assinatura RSA/ECDSA (que teria 256+ bytes de entropia alta). Parece um **trailer de metadados ou checksum**, não uma assinatura criptográfica.

## As 4 perguntas

### 1. Está criptografado? **Não.** (confiança: alta)
Entropia global de 6,58 bits/byte e média por bloco de 5,58; **nenhum** bloco de 4 KB passa de 7,9. Há 4 392 strings ASCII legíveis espalhadas por todo o arquivo (mensagens de depuração, nomes de efeitos, caminhos de código-fonte). Um firmware criptografado teria entropia uniforme perto de 8 e quase nenhuma string. Aqui não é o caso.

### 2. Está assinado? **Provavelmente não** (ou com verificação fraca). (confiança: média)
Não há bloco final de entropia alta e tamanho fixo típico de assinatura. A cauda tem só 136 bytes. O binwalk achou "SHA256 hash constants" no código — isso é a **implementação** de SHA-256 embutida, usada provavelmente para verificar **integridade** (checksum/hash), não autenticidade. Não dá para descartar uma verificação de assinatura feita pelo bootloader, mas no arquivo não há sinal de assinatura. Consequência: a troca de firmware pela via oficial de atualização **pode ser viável** se o bootloader só checar integridade. A confirmar na Fase B.

### 3. Está comprimido? **Em parte.** (confiança: média)
A maior parte do arquivo é código e dados não comprimidos (entropia média, strings legíveis). Há **regiões isoladas de entropia alta (>7,5)** espalhadas (ex.: 0x17a000–0x187000, e vários blocos de 4–20 KB mais ao fim) — provavelmente **recursos comprimidos ou dados de áudio/IR/captura**. Há também um **GIF embutido** (logo de boot, 160×34) em ~0x179000. Não há compressão do arquivo inteiro.

### 4. Qual o conjunto de instruções provável? **Não totalmente determinado; indícios de núcleo RISC não-ARM, possivelmente Andes NDS32.** (confiança: baixa/média)
- O cpu_rec, por regiões de 128 KB, aponta **ARMhf** no início (0–256 KB) e **NDS32 (Andes)** na maior região de código (≈640 KB–1,4 MB). A contagem bruta deu 378 janelas NDS32 contra poucas de ARM.
- As strings mostram caminhos de código-fonte com estilo Windows (`..\Source\driver\spi.c`, `..\Source\driver\uart.c`) e de componentes de interface (`../UserSources/Components/Encoder/…`, `WinManager`). Há uma **pilha BLE BTstack** completa (`hci_ble_send_cmd`, `BTSTACK_EVENT_…`, `le_central_…`), rotinas de **USB device**, **SDIO/cartão** e **áudio** (`DAC0 DACX All Disable`, `SamplesPreFrame`, `AudioEffectsAllDisable`).
- **Leitura:** este `.bin` é a firmware do **SoC principal**, que faz Bluetooth + USB + áudio + efeitos num só lugar. Isso casa com o **MVSilicon BP1048B2** (BLE + codec + DSP). O datasheet descreve o BP1048B2 como "núcleo RISC de 32 bits com DSP e FPU" sem nomear o núcleo; o resultado NDS32 do cpu_rec sugere que esse núcleo seja **Andes**. Se confirmado, o compilador é a toolchain **Andes (nds32/AndeSight, com GCC)**, menos comum que ARM.
- **Ambiguidade importante:** não está claro se este arquivo cobre também o chip **JAT**, ou se o JAT tem firmware próprio em memória separada. O pacote tem várias seções; uma delas pode ser destinada a um segundo processador. A confirmar na Fase B (dump dos chips e comparação com `mapa-flash.md`).

## O que isso significa para o Codex

- **Boa notícia:** o firmware **não é criptografado** e aparenta não ser assinado, então estudá-lo e, no futuro, substituí-lo pela via de atualização é plausível.
- **Atenção:** se o núcleo principal for **Andes NDS32** (e não ARM), o Codex precisa de um backend de compilador para Andes. Isso reforça a arquitetura portátil (HAL): o DSP em C portátil roda em qualquer núcleo; só a camada de baixo nível muda.
- **Próximo passo decisivo (B2/B3/A9+):** ler o IDCODE do chip pela porta de depuração (J7) para confirmar o núcleo sem depender de palpite. Um ARM responde um IDCODE conhecido; um Andes/JTAG responde outro.

## Pendências

- Rodar `strings` categorizado por seção para mapear qual seção é código, recurso e áudio (feito em parte; detalhar no `mapa-flash.md` da Fase B).
- Comparar as seções deste pacote com o dump real da flash (B7) para ver o que é extra (bootloader, presets, IRs).
