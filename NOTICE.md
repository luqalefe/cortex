# Cortex — licença e créditos de terceiros

**Cortex** é distribuído sob a **GNU GPL versão 2** (ver `LICENSE`).

A GPLv2 foi escolhida para ser compatível com os projetos abaixo, que o Cortex usa como
base/backend. (A GPLv3 não é compatível com código GPLv2-only; por isso o Cortex é GPLv2.)

## Projetos de terceiros usados

- **OpenGP5** — https://github.com/OpenGP5/OpenGP5 — licença **MIT**.
  Usado como backend do alvo GP-5 (platform dos dois processadores, HAL, flasher por USB).
  MIT é compatível com a GPLv2.

- **GuitarPedal** (Linus Torvalds) — https://github.com/torvalds/GuitarPedal — licença **GPLv2**.
  Fonte de referência e de implementações de efeitos (DSP) e primitivas de áudio
  (biquad, LFO, envelope, etc.), a serem portadas para o contrato de efeito do Cortex.
  GPLv2-only; por isso o Cortex é GPLv2.

Cada trecho aproveitado de um desses projetos mantém o aviso de licença/autoria original no
arquivo correspondente. Nada da Valeton (código, binários ou dados de firmware) entra no
repositório.
