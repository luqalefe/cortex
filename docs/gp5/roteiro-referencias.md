# Roteiro — gravações de referência do firmware original (A3) [HUMANO]

> Faça isto **antes** de qualquer outra coisa que mexa na GP-5. Depois que o firmware for trocado, essas gravações não podem mais ser refeitas.

Tudo fica em `~/codex-private/referencias/`, fora do repositório.

## 0. Antes de começar

1. Anote a **versão do firmware** da GP-5 (menu do aparelho ou editor oficial) e a **versão do editor**.
2. Confira se a GP-5 tem uma opção de **reamp/USB return** nos ajustes globais. Em geral, ela fica num menu de áudio USB (nomes como "USB Reamp", "Rec Mode", "USB In"). Hoje, sem essa opção, o áudio que o Mac manda pelo USB **não entra nos efeitos** (teste de 2026-11-14 no diário).
   - **Se existir:** ligue a opção, use o modo `usb` e pule o passo 2.
   - **Se não existir:** use o modo `analogico`, abaixo.

## 1. DI da guitarra (3 trechos)

Grave a guitarra limpa direto numa interface de áudio (DI), em **44,1 kHz / 24 bits**, com cerca de 30 s cada:

- `~/codex-private/referencias/di/1-notas.wav`: notas soltas, com sustain
- `~/codex-private/referencias/di/2-acordes.wav`: acordes abertos e com pestana
- `~/codex-private/referencias/di/3-solo.wav`: frases com bends e vibrato

O pico deve ficar entre −12 e −6 dBFS. Não use normalização nem efeitos. Se gravar no Logic, exporte em 44,1 kHz / 24 bits, mono.

## 2. Modo analógico: ligação e loopback

Ligação: **saída do Mac** (fone ou interface) → cabo P10 → **input de instrumento da GP-5**. O retorno processado vem pela **entrada USB da GP-5** ("GP-5 Audio").

1. Volume do Mac **baixo** (comece em ~30%). Esse volume **não pode mudar** durante toda a sessão. Anote o valor.
2. **Loopback:** para medir a latência da saída do Mac, ligue a saída do Mac direto na entrada da interface de áudio e rode:
   ```
   re/.venv/bin/python tools/test-signals.py --out ~/codex-private/referencias/sinais
   re/.venv/bin/python tools/codex-reamp.py --out-dev "<saída do Mac>" --in-dev "<interface>" \
       --input ~/codex-private/referencias/sinais/impulso.wav --repeat 5
   ```
   Anote a média em amostras. A latência só da GP-5 é a do teste seguinte menos esse valor da interface.
3. **Latência da GP-5 (aceite da A2):** selecione um preset com **todos os blocos desligados (bypass)**, ligue como no início deste passo e rode:
   ```
   re/.venv/bin/python tools/codex-reamp.py --out-dev "<saída do Mac>" --in-dev "GP-5" \
       --input ~/codex-private/referencias/sinais/impulso.wav --repeat 5
   ```
   Cole a saída no diário. Ela traz média, desvio e variação das 5 repetições.

## 3. Escolha de 10 presets de fábrica

Preencha `~/codex-private/referencias/presets.json` com os 10 escolhidos e os slots:

```json
[
  {"slot": "01A", "nome": "...", "tipo": "limpo"},
  {"slot": "...", "nome": "...", "tipo": "crunch"},
  {"slot": "...", "nome": "...", "tipo": "alto ganho"},
  {"slot": "...", "nome": "...", "tipo": "delay"},
  {"slot": "...", "nome": "...", "tipo": "reverb"},
  {"slot": "...", "nome": "...", "tipo": "modulação"}
]
```

Sugestão: 2 limpos, 2 crunch, 2 de alto ganho, 2 com delay em destaque, 1 com reverb e 1 com modulação. Não mexa em nenhum parâmetro dos presets.

## 4. Gravação em lote

```
cd ~/code/codex
re/.venv/bin/python tools/reamp-batch.py \
    --presets ~/codex-private/referencias/presets.json \
    --out-dev "<saída do Mac>" --in-dev "GP-5" --mode analogico \
    --latency-comp <média do loopback> --fw <versão> --editor <versão>
```

O script pede a troca de preset antes de cada um. Para cada preset, ele passa os 3 trechos + impulso + varredura, salva em `referencias/<slot>/` e atualiza o `manifest.json` com o SHA-256 de cada arquivo. Se algo falhar, rode de novo: o script retoma de onde parou.

## 5. Cópia em nuvem

Copie a pasta `~/codex-private/referencias/` inteira para a nuvem e me avise com o nome do serviço e da pasta. Só então a A3 conta como aceita.
