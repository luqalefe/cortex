#!/usr/bin/env python3
"""Reamp: toca um .wav numa saída, grava uma entrada ao mesmo tempo e salva o resultado alinhado.

Uso:
  python tools/codex-reamp.py --list
  python tools/codex-reamp.py --out-dev "GP-5" --in-dev "GP-5" --sr 44100 \
      --input sinais/impulso.wav --output rec.wav [--in-ch 1] [--out-ch 1 2] \
      [--latency-comp N] [--repeat 5]

- A gravação bruta é alinhada descontando a latência (em amostras) medida por correlação cruzada
  entre o sinal enviado e o gravado, ou um valor fixo (--latency-comp), como a latência do
  loopback do Mac no modo analógico.
- Sempre imprime a latência medida (amostras e ms). Com --repeat N, repete N vezes e mostra
  média, desvio padrão, mínimo e máximo (bom para impulsos).
- O arquivo salvo tem o mesmo tamanho do de entrada. A gravação bruta sai em <output>.raw.wav.
"""
import argparse
import sys
import numpy as np
import sounddevice as sd
import soundfile as sf
from scipy.signal import correlate


def find_dev(name, kind):
    if name is None:
        return None
    if name.isdigit():
        return int(name)
    for i, d in enumerate(sd.query_devices()):
        if name.lower() in d["name"].lower() and d[f"max_{kind}_channels"] > 0:
            return i
    sys.exit(f"dispositivo de {kind} '{name}' não encontrado (use --list)")


def measure_latency(sent, rec):
    """Atraso (em amostras) de rec em relação a sent, pelo pico da correlação cruzada."""
    c = correlate(rec, sent, mode="full", method="fft")
    lag = int(np.argmax(np.abs(c))) - (len(sent) - 1)
    peak = np.max(np.abs(c)) / (np.linalg.norm(sent) * np.linalg.norm(rec) + 1e-12)
    return lag, peak


def run_once(x, sr, out_dev, in_dev, out_ch, in_ch, tail_s):
    n_out = max(out_ch)
    buf = np.zeros((len(x) + int(sr * tail_s), n_out), dtype="float32")
    for c in out_ch:
        buf[: len(x), c - 1] = x
    rec = sd.playrec(buf, samplerate=sr, device=(in_dev, out_dev),
                     input_mapping=[in_ch], channels=None, blocking=True)
    return rec[:, 0].astype(np.float64)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--out-dev")
    ap.add_argument("--in-dev")
    ap.add_argument("--sr", type=int, default=44100)
    ap.add_argument("--input")
    ap.add_argument("--output")
    ap.add_argument("--out-ch", type=int, nargs="+", default=[1, 2])
    ap.add_argument("--in-ch", type=int, default=1)
    ap.add_argument("--tail-s", type=float, default=1.0, help="tempo extra gravado após o fim")
    ap.add_argument("--latency-comp", type=int, help="desconto fixo em amostras (sem medir)")
    ap.add_argument("--repeat", type=int, default=1)
    a = ap.parse_args()
    if a.list:
        print(sd.query_devices())
        return
    x, sr_file = sf.read(a.input, always_2d=True)
    x = x[:, 0]
    if sr_file != a.sr:
        sys.exit(f"o arquivo está em {sr_file} Hz, mas --sr é {a.sr}; converta antes")
    out_dev, in_dev = find_dev(a.out_dev, "output"), find_dev(a.in_dev, "input")
    lags = []
    rec = None
    for i in range(a.repeat):
        rec = run_once(x, a.sr, out_dev, in_dev, a.out_ch, a.in_ch, a.tail_s)
        lag, peak = measure_latency(x, rec)
        lags.append(lag)
        print(f"[{i + 1}/{a.repeat}] latência {lag} amostras = {lag / a.sr * 1000:.3f} ms "
              f"(correlação normalizada {peak:.3f})")
    if a.repeat > 1:
        L = np.array(lags)
        print(f"média {L.mean():.1f} amostras ({L.mean() / a.sr * 1000:.3f} ms), "
              f"desvio {L.std():.2f}, mín {L.min()}, máx {L.max()}, "
              f"variação {L.max() - L.min()} amostras ({(L.max() - L.min()) / a.sr * 1000:.3f} ms)")
    shift = a.latency_comp if a.latency_comp is not None else lags[-1]
    shift = max(shift, 0)
    aligned = rec[shift: shift + len(x)]
    aligned = np.pad(aligned, (0, len(x) - len(aligned)))
    if a.output:
        sf.write(a.output, aligned, a.sr, subtype="PCM_24")
        sf.write(a.output.rsplit(".", 1)[0] + ".raw.wav", rec, a.sr, subtype="PCM_24")
        print(f"salvo {a.output} (alinhado, desconto {shift} amostras)")


if __name__ == "__main__":
    main()
