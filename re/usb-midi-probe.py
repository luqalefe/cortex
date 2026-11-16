#!/usr/bin/env python3
"""Sondagem SÓ-LEITURA da USB-MIDI da GP-5.

Não grava nada, não entra em modo update. Apenas:
  1) escuta o que o pedal manda sozinho (SysEx espontâneo), por alguns segundos;
  2) envia a consulta de estado [0x12,0x30] (a mesma do gp5tool, pura leitura) e espera resposta;
  3) decodifica e mostra os quadros com o framing do OpenGP5.

Uso: re/.venv/bin/python re/usb-midi-probe.py
"""
import sys, time, pathlib
OPENGP5 = pathlib.Path.home() / "dev/sdk/OpenGP5"
sys.path.insert(0, str(OPENGP5 / "tools"))
import mido
from gp5_proto import CLASS_CMD, build_sysex, parse_sysex

PORT = "GP-5"

def show(raw, tag):
    try:
        cls, idx, ln, pl, ok = parse_sysex(bytes(raw))
        print(f"  [{tag}] cls=0x{cls:02x} idx={idx} len={ln} payload={pl.hex()} crc={'ok' if ok else 'RUIM'}")
    except Exception as e:
        print(f"  [{tag}] SysEx bruto {bytes(raw).hex()}  (não decodificou: {e})")

def main():
    ins = mido.get_input_names(); outs = mido.get_output_names()
    pin = next((p for p in ins if PORT in p), None)
    pout = next((p for p in outs if PORT in p), None)
    if not pin or not pout:
        print("GP-5 não encontrada na USB-MIDI. IN=", ins, "OUT=", outs); return
    print(f"IN='{pin}'  OUT='{pout}'")

    with mido.open_input(pin) as inp, mido.open_output(pout) as outp:
        # drena qualquer coisa pendente
        for _ in inp.iter_pending(): pass

        print("\n1) Escuta passiva (4 s) — SysEx espontâneo do pedal:")
        t0 = time.time(); spont = 0
        while time.time() - t0 < 4.0:
            for m in inp.iter_pending():
                if m.type == "sysex":
                    spont += 1; show([0xF0, *m.data, 0xF7], "espontâneo")
            time.sleep(0.01)
        print(f"   -> {spont} SysEx espontâneos em 4 s")

        print("\n2) Consulta de estado [12 30] (só leitura) e espera 1,5 s por resposta:")
        q = build_sysex(CLASS_CMD, 0, [0x12, 0x30])
        outp.send(mido.Message("sysex", data=list(q[1:-1])))
        t0 = time.time(); got = 0
        while time.time() - t0 < 1.5:
            for m in inp.iter_pending():
                if m.type == "sysex":
                    got += 1; show([0xF0, *m.data, 0xF7], "resposta")
            time.sleep(0.01)
        print(f"   -> {got} respostas à consulta de estado")

        print("\n3) Veredito:")
        if got:
            print("   O pedal RESPONDE ao handshake -> protocolo compatível; flash deve ser viável.")
        elif spont:
            print("   NÃO responde ao 12 30, mas FALA sozinho (SysEx espontâneo): canal USB-MIDI vivo,")
            print("   só o handshake difere. Precisa entender o protocolo deste pedal antes de gravar.")
        else:
            print("   Silêncio total: nem responde nem fala. Pode ser porta ocupada, modo, ou firmware diferente.")

if __name__ == "__main__":
    main()
