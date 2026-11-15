# Pedais do Cortex — faça o seu

Cada pedal é **um arquivo JSON** nesta pasta. A interface lê estes arquivos e monta a
pedaleira sozinha — então criar um pedal novo é **copiar um JSON e editar**. Não precisa
mexer no código da interface.

## Criar um pedal

1. Copie um existente, ex.: `cp ts9.json meu-fuzz.json`.
2. Edite os campos (veja abaixo).
3. Adicione o `id` dele à lista `library` (e, se quiser já na board, a `board`) em
   `manifest.json`.
4. Recarregue a interface.

## Formato

```json
{
  "id": "ts9",                    // único; casa com o DSP (effects/<id>) quando houver áudio
  "name": "TS9",                  // nome grande no pedal
  "subtitle": "Overdrive",        // linha menor
  "color": "#2ec27e",            // cor de destaque (knobs, LED)
  "dsp": "ts9",                   // qual efeito portátil toca (effects/<dsp>); opcional por ora
  "modes": ["Digital","Analog"], // opcional: se tiver, aparece um seletor de modo
  "knobs": [
    { "id": "drive", "label": "Drive", "min": 0, "max": 10, "def": 5, "unit": "" }
  ]
}
```

- **knobs:** `min`/`max` definem a faixa; `def` o padrão; `unit` é só o rótulo (ms, %, s…).
- **modes:** lista de nomes. A interface mostra um seletor; o valor escolhido entra no preset
  como `mode`. (Como cada modo soa é trabalho do DSP, feito depois.)

## Presets

Um **preset** guarda o estado da board inteira (quais pedais, ligados/desligados, modo e
knobs de cada um) com um nome. Salve/あcarregue pela interface; ficam no navegador. Dá para
exportar/importar como JSON para compartilhar.

## Pedais que já vêm

- `ts9` — overdrive (Drive/Tone/Level).
- `delay` — delay estilo DL4, sem looper (modos + Time/Repeats/Tweak/Tweez/Mix).
- `reverb` — reverb estilo RV-6 (modos + Level/Tone/Decay).

Os três são exemplos: edite à vontade ou use de base para os seus.
