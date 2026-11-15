#!/usr/bin/env python3
"""Lista os descritores USB da Valeton GP-5 (somente leitura).

Uso: re/.venv/bin/python re/usb-enum.py [--vid 0x84ef] [--pid 0x0184]
Não envia nenhum comando ao aparelho além das leituras de descritor padrão.
"""
import argparse
import usb.core
import usb.util
import usb.backend.libusb1

CLASSES = {0x00: "por-interface", 0x01: "Áudio", 0x02: "CDC", 0x03: "HID",
           0x08: "Mass Storage", 0x0A: "CDC-Data", 0xEF: "Misc", 0xFE: "App-Specific",
           0xFF: "Vendor-Specific"}
AUDIO_SUB = {0x01: "AudioControl", 0x02: "AudioStreaming", 0x03: "MIDIStreaming"}
XFER = {0: "controle", 1: "isócrono", 2: "bulk", 3: "interrupção"}


def backend():
    for path in ("/opt/homebrew/lib/libusb-1.0.dylib", "/usr/local/lib/libusb-1.0.dylib"):
        try:
            b = usb.backend.libusb1.get_backend(find_library=lambda _x, p=path: p)
            if b:
                return b
        except Exception:
            pass
    return None


def s(dev, idx):
    if not idx:
        return ""
    try:
        return usb.util.get_string(dev, idx)
    except Exception as e:  # macOS às vezes nega strings sem claim
        return f"<{e.__class__.__name__}>"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vid", type=lambda x: int(x, 0), default=0x84EF)
    ap.add_argument("--pid", type=lambda x: int(x, 0), default=0x0184)
    a = ap.parse_args()
    dev = usb.core.find(idVendor=a.vid, idProduct=a.pid, backend=backend())
    if dev is None:
        raise SystemExit(f"Dispositivo {a.vid:04x}:{a.pid:04x} não encontrado")
    print(f"VID:PID      {dev.idVendor:04x}:{dev.idProduct:04x}")
    print(f"Fabricante   {s(dev, dev.iManufacturer)}")
    print(f"Produto      {s(dev, dev.iProduct)}")
    print(f"Serial       {s(dev, dev.iSerialNumber)}")
    print(f"bcdUSB       {dev.bcdUSB:04x}  bcdDevice {dev.bcdDevice:04x}")
    print(f"Classe disp. 0x{dev.bDeviceClass:02x} ({CLASSES.get(dev.bDeviceClass, '?')}) "
          f"sub 0x{dev.bDeviceSubClass:02x} proto 0x{dev.bDeviceProtocol:02x}")
    print(f"EP0 máx      {dev.bMaxPacketSize0}")
    for cfg in dev:
        print(f"\nConfiguração {cfg.bConfigurationValue}: {cfg.bNumInterfaces} interfaces, "
              f"{cfg.bMaxPower * 2} mA")
        for intf in cfg:
            sub = AUDIO_SUB.get(intf.bInterfaceSubClass, "") if intf.bInterfaceClass == 1 else ""
            print(f"  Interface {intf.bInterfaceNumber} alt {intf.bAlternateSetting}: "
                  f"classe 0x{intf.bInterfaceClass:02x} ({CLASSES.get(intf.bInterfaceClass, '?')}) "
                  f"sub 0x{intf.bInterfaceSubClass:02x} {sub} proto 0x{intf.bInterfaceProtocol:02x} "
                  f"'{s(dev, intf.iInterface)}'")
            for ep in intf:
                d = "IN " if ep.bEndpointAddress & 0x80 else "OUT"
                print(f"    EP 0x{ep.bEndpointAddress:02x} {d} {XFER[ep.bmAttributes & 3]:<11} "
                      f"máx {ep.wMaxPacketSize} intervalo {ep.bInterval}")


if __name__ == "__main__":
    main()
