#!/usr/bin/env python3
"""Conecta à GP-5 por BLE e lista os serviços/características (GATT).

Uso: re/.venv/bin/python re/ble-connect.py [nome|endereço]
Padrão: procura "GP-5 BLE".

Leitura:
  - Conectou e listou serviços -> rádio + stack + GATT OK; defeito está no app/celular.
  - Achou anunciando mas NÃO conectou -> problema mais estreito (aceitar conexão).
Serviço BLE-MIDI padrão = 03B80E5A-EDE8-4B33-A751-6CE34EC4C700 (se existir, dá p/ controlar sem o app).
"""
import asyncio, sys
from bleak import BleakScanner, BleakClient

TARGET = sys.argv[1] if len(sys.argv) > 1 else "GP-5 BLE"
MIDI_SVC = "03b80e5a-ede8-4b33-a751-6ce34ec4c700"

async def main():
    print(f"Procurando '{TARGET}'...")
    dev = None
    if "-" in TARGET and len(TARGET) >= 17 and TARGET.count("-") >= 4:
        dev = await BleakScanner.find_device_by_address(TARGET, timeout=15)
    else:
        dev = await BleakScanner.find_device_by_name(TARGET, timeout=15)
    if not dev:
        print("Não encontrado anunciando. Ligue o BT no pedal (HOME) e tente de novo.")
        return
    print(f"Achado: {dev.address}  nome='{dev.name}'. Conectando...")
    try:
        async with BleakClient(dev, timeout=20) as cli:
            print(f"CONECTADO: {cli.is_connected}\n")
            for s in cli.services:
                print(f"serviço {s.uuid}  ({s.description})")
                for c in s.characteristics:
                    props = ",".join(c.properties)
                    print(f"   char {c.uuid}  [{props}]  ({c.description})")
                    if "read" in c.properties:
                        try:
                            v = await cli.read_gatt_char(c.uuid)
                            txt = v.decode("utf-8", "replace").strip()
                            print(f"        valor: {v.hex()}  '{txt}'")
                        except Exception as e:
                            print(f"        (não leu: {e})")
            has_midi = any(s.uuid.lower() == MIDI_SVC for s in cli.services)
            print("\nServiço BLE-MIDI padrão:", "SIM" if has_midi else "não")
            print("=> Conexão BLE funcionou: rádio + stack + GATT OK. O defeito está no app/celular.")
    except Exception as e:
        print(f"NÃO conectou: {e}")
        print("=> Anuncia mas recusa conexão: problema de software mais estreito, não de rádio.")

if __name__ == "__main__":
    asyncio.run(main())
