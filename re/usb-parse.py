#!/usr/bin/env python3
"""Extrai e compara o tráfego USB da GP-5 em capturas .pcapng (USBPcap/Windows ou macOS).

Uso:
  python re/usb-parse.py dump  CAPTURA.pcapng [--sysex] [--addr N]
  python re/usb-parse.py diff  A.pcapng B.pcapng [--sysex]
  python re/usb-parse.py check CAPTURA.pcapng            # procura cabeçalho/tamanho/contador/checksum

Requer `tshark` (Wireshark). As capturas ficam em ~/codex-private/capturas-usb/ (fora do repo).
- dump: lista as transferências (tempo, direção, endpoint, tipo, tamanho, dados em hex).
- --sysex: remonta os pacotes USB-MIDI (4 bytes por evento, CIN 0x4–0x7) em mensagens SysEx F0…F7.
- --addr: filtra pelo endereço USB do aparelho (se omitido, usa o mais frequente com dados).
- diff: alinha as mensagens das duas capturas por ordem e mostra só os bytes que mudaram.
- check: testa soma, XOR, CRC-8, CRC-16 (CCITT/IBM/Modbus) e CRC-32 contra o fim de cada
  mensagem, e procura campos de tamanho e contadores.
"""
import argparse
import collections
import subprocess
import sys
import zlib

XFER = {"0": "isócrono", "1": "interrupção", "2": "controle", "3": "bulk"}
FIELDS = ["frame.time_relative", "usb.src", "usb.dst", "usb.endpoint_address",
          "usb.transfer_type", "usb.data_len", "usb.capdata", "usb.device_address"]


def tshark(path):
    cmd = ["tshark", "-r", path, "-T", "fields", "-E", "separator=\t", "-E", "occurrence=f"]
    for f in FIELDS:
        cmd += ["-e", f]
    out = subprocess.run(cmd, capture_output=True, text=True)
    if out.returncode:
        sys.exit(out.stderr)
    rows = []
    for line in out.stdout.splitlines():
        p = (line.split("\t") + [""] * len(FIELDS))[: len(FIELDS)]
        data = bytes.fromhex(p[6].replace(":", "")) if p[6] else b""
        if not data:
            continue
        ep = int(p[3], 16) if p[3] else 0
        rows.append(dict(t=float(p[0] or 0), src=p[1], dst=p[2], ep=ep,
                         dir="IN " if ep & 0x80 else "OUT", type=XFER.get(p[4], p[4]),
                         data=data, addr=p[7]))
    return rows


def pick_addr(rows, addr):
    if addr:
        return [r for r in rows if r["addr"] == str(addr)]
    c = collections.Counter(r["addr"] for r in rows if r["type"] != "isócrono")
    if not c:
        return rows
    best = c.most_common(1)[0][0]
    return [r for r in rows if r["addr"] == best]


def sysex(rows):
    """Remonta SysEx de eventos USB-MIDI (4 bytes: cabeçalho CIN + 3 bytes MIDI)."""
    msgs, cur = [], {}
    for r in rows:
        if r["type"] != "bulk":
            continue
        d = r["data"]
        for i in range(0, len(d) - 3, 4):
            cin, b = d[i] & 0x0F, d[i + 1: i + 4]
            key = r["dir"]
            n = {0x4: 3, 0x5: 1, 0x6: 2, 0x7: 3}.get(cin)
            if n is None:
                continue
            cur.setdefault(key, bytearray()).extend(b[:n])
            if cin in (0x5, 0x6, 0x7):
                msgs.append(dict(t=r["t"], dir=key, ep=r["ep"], type="sysex", data=bytes(cur.pop(key))))
    return msgs


def hx(b):
    return " ".join(f"{x:02X}" for x in b)


def load(path, a):
    rows = pick_addr(tshark(path), a.addr)
    rows = [r for r in rows if r["type"] != "isócrono"]  # áudio não interessa aqui
    return sysex(rows) if a.sysex else rows


def cmd_dump(a):
    for r in load(a.files[0], a):
        print(f"{r['t']:10.4f} {r['dir']} EP 0x{r['ep']:02x} {r['type']:<11} {len(r['data']):4d}  {hx(r['data'])}")


def cmd_diff(a):
    A, B = load(a.files[0], a), load(a.files[1], a)
    print(f"A: {len(A)} mensagens   B: {len(B)} mensagens")
    for i, (x, y) in enumerate(zip(A, B)):
        dx, dy = x["data"], y["data"]
        if dx == dy:
            continue
        ch = [j for j in range(max(len(dx), len(dy)))
              if (dx[j] if j < len(dx) else None) != (dy[j] if j < len(dy) else None)]
        desc = ", ".join(f"@{j}: {dx[j] if j < len(dx) else '-':>3} -> {dy[j] if j < len(dy) else '-':>3}"
                         for j in ch[:24])
        print(f"#{i} {x['dir']} tam {len(dx)}/{len(dy)}  {len(ch)} bytes diferentes: {desc}")


def crc(data, width, poly, init, refin, xorout):
    top, mask, c = 1 << (width - 1), (1 << width) - 1, init
    for b in data:
        if refin:
            b = int(f"{b:08b}"[::-1], 2)
        c ^= b << (width - 8)
        for _ in range(8):
            c = ((c << 1) ^ poly) if c & top else (c << 1)
            c &= mask
    if refin:
        c = int(f"{c:0{width}b}"[::-1], 2)
    return c ^ xorout


CHECKS = {
    "soma8": lambda d: sum(d) & 0xFF,
    "soma7": lambda d: sum(d) & 0x7F,
    "soma-complemento7": lambda d: (-sum(d)) & 0x7F,
    "xor8": lambda d: __import__("functools").reduce(lambda x, y: x ^ y, d, 0),
    "crc8": lambda d: crc(d, 8, 0x07, 0, False, 0),
    "crc8-maxim": lambda d: crc(d, 8, 0x31, 0, True, 0),
    "crc16-ccitt": lambda d: crc(d, 16, 0x1021, 0xFFFF, False, 0),
    "crc16-ibm": lambda d: crc(d, 16, 0x8005, 0, True, 0),
    "crc16-modbus": lambda d: crc(d, 16, 0x8005, 0xFFFF, True, 0),
    "crc32": lambda d: zlib.crc32(d),
}


def candidates(v, width):
    """Formas como um checksum pode aparecer: bytes LE/BE e, em SysEx, nibbles de 4 bits."""
    n = width // 8
    out = {"le": v.to_bytes(n, "little"), "be": v.to_bytes(n, "big")}
    nib = bytes((v >> (4 * k)) & 0xF for k in range(2 * n))
    out["nib-le"], out["nib-be"] = nib, nib[::-1]
    if width == 8:
        out["7bit"] = bytes([v & 0x7F])
    return out


def cmd_check(a):
    msgs = load(a.files[0], a)
    payloads = [m["data"][1:-1] if m["data"][:1] == b"\xF0" else m["data"] for m in msgs]
    print(f"{len(payloads)} mensagens")
    lens = collections.Counter(len(p) for p in payloads)
    print("tamanhos mais comuns:", lens.most_common(6))
    if payloads:
        k = min(len(p) for p in payloads)
        fixed = [j for j in range(k) if len({p[j] for p in payloads}) == 1]
        print("posições constantes (cabeçalho fixo?):", fixed[:32])
        for j in range(min(k, 16)):
            for p_w in (1, 2):
                vals = [int.from_bytes(p[j:j + p_w], "little") for p in payloads if len(p) > j + p_w]
                for off in range(-4, 5):
                    if vals and all(v == len(p) + off for v, p in zip(vals, payloads)):
                        print(f"campo de tamanho provável em @{j} ({p_w} B, len{off:+d})")
            seq = [p[j] for p in payloads if len(p) > j]
            if len(seq) > 3 and all((y - x) & 0xFF == 1 for x, y in zip(seq, seq[1:])):
                print(f"contador de sequência provável em @{j}")
    hits = collections.Counter()
    for p in payloads:
        for name, fn in CHECKS.items():
            width = 32 if name == "crc32" else 16 if name.startswith("crc16") else 8
            for tl in range(1, 9):
                body, tail = p[:-tl], p[-tl:]
                for start in range(0, min(12, len(body))):
                    for form, enc in candidates(fn(body[start:]), width).items():
                        if enc == tail:
                            hits[(name, form, start, tl)] += 1
    print("checksums que batem (nome, forma, início, bytes no fim): ocorrências")
    for k, v in hits.most_common(10):
        print(f"  {k}: {v}/{len(payloads)}")
    if not hits:
        print("  nenhum")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["dump", "diff", "check"])
    ap.add_argument("files", nargs="+")
    ap.add_argument("--sysex", action="store_true")
    ap.add_argument("--addr")
    a = ap.parse_args()
    {"dump": cmd_dump, "diff": cmd_diff, "check": cmd_check}[a.cmd](a)


if __name__ == "__main__":
    main()
