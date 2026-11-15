#!/usr/bin/env python3
"""Analisa um arquivo oficial de atualização da GP-5 (somente leitura; nada é copiado ao repo).

Uso: re/.venv/bin/python re/fw-analyze.py ~/codex-private/firmware/ARQUIVO [--block 4096]

Saídas (tudo em ~/codex-private/firmware/<nome>.analise/):
  binwalk.txt, entropia.csv, entropia.png (se matplotlib existir), strings-categorias.txt,
  cpu_rec.txt, resumo.txt (texto descritivo, sem conteúdo do firmware, para basear o laudo).
"""
import argparse
import collections
import hashlib
import math
import os
import re
import subprocess
import sys
import zlib

CPU_REC = os.path.expanduser("~/dev/sdk/cpu_rec/cpu_rec.py")
CATS = {
    "versões": re.compile(rb"\bv?\d+\.\d+(\.\d+)?\b"),
    "mensagens de erro": re.compile(rb"(?i)error|fail|invalid|timeout|assert"),
    "nomes de efeitos/áudio": re.compile(rb"(?i)amp|cab|delay|reverb|chorus|drive|fuzz|comp|eq|"
                                         rb"phaser|flanger|wah|gate|tremolo|preset|patch|tuner"),
    "boot/atualização": re.compile(rb"(?i)boot|update|upgrade|flash|dfu|ota|firmware|crc|checksum"),
    "SO/RTOS/bibliotecas": re.compile(rb"(?i)freertos|rtos|linux|uboot|u-boot|newlib|lwip|zephyr|"
                                      rb"rt-thread|cmsis|hal_|stm32|esp|bluetooth|ble"),
    "caminhos/arquivos": re.compile(rb"[/\\][\w.-]+\.(c|h|cpp|bin|wav|ir)\b"),
}


def entropy(b):
    if not b:
        return 0.0
    c = collections.Counter(b)
    n = len(b)
    return abs(-sum(v / n * math.log2(v / n) for v in c.values()))


def strings(data, minlen=5):
    return re.findall(rb"[\x20-\x7e]{%d,}" % minlen, data)


def run(cmd):
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
        return r.stdout + r.stderr
    except Exception as e:
        return f"falhou: {e}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--block", type=int, default=4096)
    a = ap.parse_args()
    path = os.path.abspath(os.path.expanduser(a.path))
    if os.path.expanduser("~/codex-private") not in path:
        sys.exit("por regra, o arquivo deve estar em ~/codex-private/")
    data = open(path, "rb").read()
    out = path + ".analise"
    os.makedirs(out, exist_ok=True)
    R = []
    R.append(f"arquivo: {os.path.basename(path)}  tamanho: {len(data)} B  "
             f"sha256: {hashlib.sha256(data).hexdigest()}")
    R.append(f"entropia global: {entropy(data):.4f} bits/byte")

    # binwalk
    bw = run(["binwalk", path])
    open(f"{out}/binwalk.txt", "w").write(bw)
    sig = [l for l in bw.splitlines() if re.match(r"^\s*\d+\s+0x", l)]
    R.append(f"binwalk: {len(sig)} assinaturas")
    for l in sig[:30]:
        parts = l.split(None, 2)
        R.append(f"  @ {parts[1]}: {parts[2][:60] if len(parts) > 2 else ''}")

    # entropia por blocos
    ents = [entropy(data[i:i + a.block]) for i in range(0, len(data), a.block)]
    with open(f"{out}/entropia.csv", "w") as f:
        f.write("offset,entropia\n")
        for k, e in enumerate(ents):
            f.write(f"{k * a.block},{e:.4f}\n")
    hi = sum(e > 7.9 for e in ents)
    lo = sum(e < 1.0 for e in ents)
    R.append(f"entropia por blocos de {a.block} B: {len(ents)} blocos; mín {min(ents):.2f}, "
             f"máx {max(ents):.2f}, média {sum(ents) / len(ents):.2f}; "
             f">7,9: {hi} ({hi / len(ents):.0%}); <1,0 (padding): {lo}")
    # regiões contíguas por faixa
    def band(e):
        return "alta(>7.5)" if e > 7.5 else "media(5-7.5)" if e > 5 else "baixa(<5)"
    regs, start = [], 0
    for k in range(1, len(ents) + 1):
        if k == len(ents) or band(ents[k]) != band(ents[start]):
            regs.append((start * a.block, min(k * a.block, len(data)), band(ents[start])))
            start = k
    R.append("regiões de entropia (início-fim, faixa):")
    for s, e, b in regs[:60]:
        if e - s >= a.block * 2 or len(regs) < 40:
            R.append(f"  0x{s:08x}-0x{e:08x} ({e - s} B): {b}")
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        plt.figure(figsize=(12, 3))
        plt.plot([k * a.block for k in range(len(ents))], ents)
        plt.ylim(0, 8.1)
        plt.xlabel("offset")
        plt.ylabel("bits/byte")
        plt.title(os.path.basename(path))
        plt.tight_layout()
        plt.savefig(f"{out}/entropia.png", dpi=120)
    except ImportError:
        R.append("(matplotlib ausente: gráfico não gerado; instale no venv para gerar)")

    # strings por categoria (só contagens no resumo)
    ss = strings(data)
    R.append(f"strings (>=5 caracteres ASCII): {len(ss)}; por KB: {len(ss) / max(1, len(data) / 1024):.2f}")
    with open(f"{out}/strings-categorias.txt", "w") as f:
        for name, rx in CATS.items():
            hits = [s for s in ss if rx.search(s)]
            R.append(f"  {name}: {len(hits)}")
            f.write(f"## {name} ({len(hits)})\n")
            for s in hits[:200]:
                f.write(s.decode("ascii", "replace") + "\n")

    # cabeçalho e checksums
    R.append("cabeçalho (estrutura, sem conteúdo):")
    head = data[:64]
    magic_ascii = re.match(rb"[\x20-\x7e]{3,}", head)
    R.append(f"  começa com texto ASCII: {'sim, ' + str(len(magic_ascii.group())) + ' caracteres' if magic_ascii else 'não'}")
    # CRC-32 de cada região candidata, calculado uma vez (incremental a partir do fim)
    crcs = {}
    for tail in (0, 4, 256, 512):
        end = len(data) - tail
        suffix = {}
        for skip in range(0, 516, 4):
            suffix[skip] = data[skip:end]
        for skip, body in suffix.items():
            crcs[zlib.crc32(body)] = (skip, tail)
    for off in range(0, 64, 4):
        for endian in ("little", "big"):
            v = int.from_bytes(data[off:off + 4], endian)
            if v in (len(data), len(data) - 64, len(data) - 32, len(data) - 256, len(data) - 128,
                     len(data) - 512, len(data) - off - 4):
                R.append(f"  campo de tamanho provável @0x{off:x} ({endian}) = {v} (arquivo {len(data)})")
            if v and v in crcs:
                skip, tail = crcs[v]
                R.append(f"  CRC-32 bate @0x{off:x} ({endian}) sobre [0x{skip:x}, fim-{tail}]")
    for tail in (4,):
        v_le = int.from_bytes(data[-4:], "little")
        v_be = int.from_bytes(data[-4:], "big")
        c = zlib.crc32(data[:-4])
        if c in (v_le, v_be):
            R.append("  CRC-32 dos dados bate com os 4 bytes finais")
    for n in (128, 256, 512):
        e = entropy(data[-n:])
        R.append(f"  entropia dos últimos {n} B: {e:.2f} (assinatura RSA/ECDSA costuma ser alta, perto de log2(n))")
    for alg in ("md5", "sha1", "sha256"):
        h = hashlib.new(alg)
        dl = h.digest_size
        for pos, body in (("início", data[dl:]), ("fim", data[:-dl])):
            dig = hashlib.new(alg, body).digest()
            if (pos == "início" and data[:dl] == dig) or (pos == "fim" and data[-dl:] == dig):
                R.append(f"  hash {alg} no {pos} bate com o resto do arquivo")
            if dig in data[:1024]:
                R.append(f"  hash {alg} de [{pos}] encontrado no cabeçalho")

    # cpu_rec
    if os.path.exists(CPU_REC):
        cr = run([sys.executable, CPU_REC, "-v", path])
        open(f"{out}/cpu_rec.txt", "w").write(cr)
        R.append("cpu_rec (últimas linhas):")
        R += ["  " + l for l in cr.strip().splitlines()[-12:]]
    else:
        R.append("cpu_rec não encontrado em ~/dev/sdk/cpu_rec")

    open(f"{out}/resumo.txt", "w").write("\n".join(R) + "\n")
    print("\n".join(R))
    print(f"\nsaídas em {out}")


if __name__ == "__main__":
    main()
