# Valeton GP-5 — protocolo do editor (rascunho)

Situação: **em andamento**. Por enquanto, só o tráfego espontâneo foi observado no Mac. Faltam as capturas do editor (A5). Grau de confiança: **confirmado** (testado), **provável** (visto várias vezes) ou **hipótese**.

## Transporte

| Item | Valor | Confiança |
|---|---|---|
| Interface | 3, USB-MIDI 1.0 (classe 0x01/0x03) | confirmado (descritores) |
| Endpoints | `0x03` OUT bulk 64 B, `0x82` IN bulk 64 B | confirmado |
| Formato | SysEx (`F0 … F7`) | confirmado para mensagens do aparelho |
| Usado pelo editor | interface 3 | provável (é a única interface de dados; falta captura) |

## Observações de 2026-11-14 (Mac, CoreMIDI, sem editor)

- Sem nenhum comando enviado, a GP-5 emite **um SysEx de 16 bytes** de tempos em tempos (1 mensagem em ~6 s de escuta). Confiança: confirmado.
- Estrutura vista: `F0`, um byte de identificação que não é o ID de fabricante padrão de 3 bytes (`00 xx xx`), 13 bytes de conteúdo e `F7`. Depois do `F0`, **todos os bytes ficam entre 0x00 e 0x0F**. Hipótese: conteúdo codificado em *nibbles* (cada byte carrega 4 bits).
- O *Identity Request* universal (`F0 7E 7F 06 01 F7`) **não recebeu resposta** em 2 s. Confiança: confirmado. O aparelho não implementa a identificação universal, ou só responde dentro de outra sessão.
- Hipótese: a mensagem periódica é um *keep-alive* ou um aviso de estado (por exemplo, preset atual).

## Estrutura da mensagem

A definir com as capturas e com `re/usb-parse.py check` (cabeçalho fixo, tamanho, contador, checksum).

## Tipos de mensagem e mapa de parâmetros

A definir (capturas 03–05 e 10–14 isolam o byte de cada parâmetro).

## Comparação com a GP-200 (Preset Forge)

A fazer depois das capturas. O código e a documentação públicos do projeto Preset Forge serão consultados como **referência de formato**, com citação da fonte, sem copiar código.
