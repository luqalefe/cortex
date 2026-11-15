#!/usr/bin/env python3
"""Grava em lote as referências do firmware original da GP-5 (tarefa A3) e gera o manifest.json.

Uso:
  re/.venv/bin/python tools/reamp-batch.py --presets presets.json \
      --out-dev "Fones" --in-dev "GP-5" --mode analogico --fw 1.x.x --editor 1.x.x \
      [--latency-comp 312] [--root ~/codex-private/referencias]

presets.json (o humano preenche): [{"slot": "01A", "nome": "Clean 1", "tipo": "limpo"}, ...]

Para cada preset, o script pede ao humano para selecionar o slot na GP-5 e apertar Enter. Em
seguida, passa os 3 trechos de DI (em <root>/di/*.wav), o impulso e a varredura, sempre com o
mesmo volume de saída. Os arquivos ficam em <root>/<slot>/ e o manifest em <root>/manifest.json
(com SHA-256 de cada arquivo). Se uma gravação falhar, rode de novo: presets já completos são
pulados.
"""
import argparse
import datetime
import hashlib
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PY = sys.executable


def sha(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--presets", required=True)
    ap.add_argument("--out-dev", required=True)
    ap.add_argument("--in-dev", required=True)
    ap.add_argument("--mode", choices=["usb", "analogico"], required=True)
    ap.add_argument("--fw", required=True, help="versão do firmware da GP-5")
    ap.add_argument("--editor", required=True, help="versão do editor oficial")
    ap.add_argument("--sr", type=int, default=44100)
    ap.add_argument("--latency-comp", type=int, help="desconto fixo (loopback) em amostras")
    ap.add_argument("--in-ch", type=int, default=1)
    ap.add_argument("--root", default="~/codex-private/referencias")
    a = ap.parse_args()
    root = os.path.expanduser(a.root)
    di = sorted(f for f in os.listdir(f"{root}/di") if f.endswith(".wav"))
    if len(di) < 3:
        sys.exit(f"esperava 3 trechos de DI em {root}/di (achei {len(di)})")
    sig = f"{root}/sinais"
    if not os.path.exists(f"{sig}/impulso.wav"):
        subprocess.run([PY, f"{HERE}/test-signals.py", "--out", sig, "--sr", str(a.sr)], check=True)
    inputs = [f"{root}/di/{f}" for f in di] + [f"{sig}/impulso.wav", f"{sig}/varredura.wav"]
    presets = json.load(open(a.presets))
    man_path = f"{root}/manifest.json"
    man = json.load(open(man_path)) if os.path.exists(man_path) else {"itens": []}
    man.update(firmware=a.fw, editor=a.editor, modo_reamp=a.mode, taxa=a.sr,
               saida=a.out_dev, entrada=a.in_dev, desconto_latencia=a.latency_comp,
               sinais={os.path.relpath(p, root): sha(p) for p in inputs})
    done = {i["slot"] for i in man["itens"] if i.get("completo")}
    for p in presets:
        if p["slot"] in done:
            print(f"pulando {p['slot']} (já completo)")
            continue
        input(f"\n>>> Selecione o preset {p['slot']} ({p.get('nome', '')}) na GP-5 e aperte Enter...")
        d = f"{root}/{p['slot']}"
        os.makedirs(d, exist_ok=True)
        files = {}
        for src in inputs:
            out = f"{d}/{os.path.basename(src)}"
            cmd = [PY, f"{HERE}/codex-reamp.py", "--out-dev", a.out_dev, "--in-dev", a.in_dev,
                   "--sr", str(a.sr), "--input", src, "--output", out, "--in-ch", str(a.in_ch)]
            if a.latency_comp is not None:
                cmd += ["--latency-comp", str(a.latency_comp)]
            subprocess.run(cmd, check=True)
            for f in (out, out[:-4] + ".raw.wav"):
                files[os.path.relpath(f, root)] = sha(f)
        man["itens"] = [i for i in man["itens"] if i["slot"] != p["slot"]] + [
            dict(p, arquivos=files, completo=True, data=datetime.datetime.now().isoformat())]
        json.dump(man, open(man_path, "w"), indent=2, ensure_ascii=False)
    print(f"\nmanifest: {man_path} ({len(man['itens'])} presets)")


if __name__ == "__main__":
    main()
