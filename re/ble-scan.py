#!/usr/bin/env python3
"""Scanner BLE para diagnóstico do Bluetooth da GP-5.

A GP-5 usa BLE (o app de celular controla o pedal). Se o rádio + a pilha BLE do
firmware estão vivos, o pedal deve ANUNCIAR por BLE quando o Bluetooth está ligado
nele (segurando HOME). Este scanner mostra tudo que anuncia por perto, por ~20 s,
com nome, endereço, RSSI (potência) e dados de fabricante.

Uso:
    re/.venv/bin/python re/ble-scan.py [segundos]

Leitura:
  - Se aparecer um dispositivo com nome tipo "GP-5"/"Valeton" (ou um sem nome com
    RSSI forte que surge só quando você liga o BT do pedal) -> o rádio funciona;
    o problema provável é app/pareamento (software), não hardware.
  - Se NADA novo aparece ao ligar o BT, mesmo repetindo e reiniciando o pedal ->
    aponta para firmware-não-inicia-BLE ou hardware (rádio/antena).
"""
import asyncio, sys, time
from bleak import BleakScanner

SECS = int(sys.argv[1]) if len(sys.argv) > 1 else 20
HINTS = ("gp-5", "gp5", "valeton", "hotone", "bp1048")

seen = {}

def cb(dev, adv):
    key = dev.address
    name = adv.local_name or dev.name or ""
    mfg = ""
    if adv.manufacturer_data:
        parts = []
        for cid, data in adv.manufacturer_data.items():
            parts.append(f"0x{cid:04x}:{data.hex()}")
        mfg = " ".join(parts)
    rec = (name, adv.rssi, mfg, time.strftime("%H:%M:%S"))
    hit = any(h in (name.lower() + " " + mfg.lower()) for h in HINTS)
    if key not in seen:
        tag = "  <<< POSSÍVEL GP-5" if hit else ""
        print(f"[{rec[3]}] {key}  RSSI {adv.rssi:4d}dBm  nome='{name}'  mfg={mfg or '-'}{tag}")
        seen[key] = rec

async def main():
    print(f"Escaneando BLE por {SECS} s... (ligue o Bluetooth do pedal agora: segure HOME)")
    print("Dica: fique perto; RSSI mais perto de 0 = mais forte.\n")
    scanner = BleakScanner(detection_callback=cb)
    await scanner.start()
    await asyncio.sleep(SECS)
    await scanner.stop()
    print(f"\nFim. {len(seen)} dispositivos BLE vistos.")
    hits = [k for k, v in seen.items() if any(h in (v[0].lower()+' '+v[2].lower()) for h in HINTS)]
    if hits:
        print("Candidatos a GP-5:", ", ".join(hits))
    else:
        print("Nenhum candidato óbvio a GP-5. Se o BT estava ligado no pedal e nada novo")
        print("surgiu, repita reiniciando o pedal; persistindo, suspeita recai em hardware/rádio.")

if __name__ == "__main__":
    asyncio.run(main())
