# Roteiro de flash — Cortex na GP-5  [HUMANO]

> **Risco de brick é real.** Loader de fábrica é intacto (recuperável), mas no pior caso a
> recuperação exige um FT232H soldado nos pads internos. Só prossiga ciente disso.

Pré-checado nesta sessão:
- Pedal responde ao handshake (`12 30` -> `11 30 01 00`): protocolo compatível. ✅
- Imagem `build/GP-5_codex-delay.bin` válida (`gp5tool check`). ✅
- Sem backup da unidade (firmware de fábrica não é legível); volta = `.bin` oficial V1.0.6. ⚠️

## Gravar o Cortex

```bash
cd ~/code/codex
re/.venv/bin/python ~/dev/sdk/OpenGP5/tools/gp5tool.py flash \
    build/GP-5_codex-delay.bin --sections all
```
O tool valida a imagem, põe o pedal em modo update sozinho, grava e espera ele voltar (~40 s).
**Manter plugado e ligado o tempo todo.**

## Se algo der errado (recuperação)

1. Desplugue o pedal.
2. Segure **footswitch + knob PARA** e plugue de novo. Tela fica escura = modo update.
3. Grave de novo (o Cortex, ou a Valeton):

## Voltar para a Valeton de fábrica

```bash
re/.venv/bin/python ~/dev/sdk/OpenGP5/tools/gp5tool.py flash \
    "$HOME/codex-private/firmware/V1.0.6/GP-5 Firmware V1.0.6.bin" --sections all
```

## Depois de gravar o Cortex

- Tela deve mostrar "CORTEX DELAY" (ou "BYPASS"); knob de volume = mix, PARA = time, footswitch = bypass.
- Testar áudio com guitarra (usar fone/interface p/ evitar microfonia).
- Se a tela ficar escura ou o app não responder: ver "recuperação" acima.
