#!/usr/bin/env python3
"""Gera sinais de teste: impulso e varredura log 20 Hz–20 kHz, com 2 s de silêncio antes e depois.

Uso: python tools/test-signals.py --out DIR [--sr 44100] [--sweep-s 10] [--level-db -12]
Gera DIR/impulso.wav, DIR/varredura.wav e DIR/varredura-inversa.wav (filtro de Farina
para deconvolução, útil para obter a resposta ao impulso a partir da varredura).
"""
import argparse
import os
import numpy as np
import soundfile as sf


def pad(x, sr, s=2.0):
    z = np.zeros(int(sr * s))
    return np.concatenate([z, x, z])


def sweep(sr, dur, f1=20.0, f2=20000.0):
    t = np.arange(int(sr * dur)) / sr
    k = np.log(f2 / f1)
    x = np.sin(2 * np.pi * f1 * dur / k * (np.exp(t * k / dur) - 1))
    fade = int(sr * 0.01)
    x[:fade] *= np.linspace(0, 1, fade)
    x[-fade:] *= np.linspace(1, 0, fade)
    inv = x[::-1] * np.exp(-t * k / dur)  # compensa a ênfase em graves
    return x, inv / np.max(np.abs(inv))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--sr", type=int, default=44100)
    ap.add_argument("--sweep-s", type=float, default=10.0)
    ap.add_argument("--level-db", type=float, default=-12.0)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    g = 10 ** (a.level_db / 20)
    imp = np.zeros(64)
    imp[0] = 1.0
    sw, inv = sweep(a.sr, a.sweep_s)
    for name, x in (("impulso", imp * g), ("varredura", sw * g)):
        sf.write(os.path.join(a.out, f"{name}.wav"), pad(x, a.sr), a.sr, subtype="PCM_24")
    sf.write(os.path.join(a.out, "varredura-inversa.wav"), inv, a.sr, subtype="FLOAT")
    print(f"ok: {a.out} ({a.sr} Hz, nível {a.level_db} dBFS)")


if __name__ == "__main__":
    main()
