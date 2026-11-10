#!/usr/bin/env python
"""Tes komunikasi KHUSUS DS28E38 (1-Wire) lewat DS9481P-300. Hanya membaca.

  python test_ds28e38.py --port COM13
  python test_ds28e38.py --port COM13 --debug     (tampilkan byte TX '>' / RX '<')

Langkah (berhenti di langkah pertama yang gagal, supaya penyebabnya jelas):
  1 adaptor terdeteksi   2 versi adaptor   3 ada presence pulse di bus 1-Wire
  4 READ ROM + CRC8      5 Read Status     6 baca halaman 0-5   7 RNG
"""

import argparse
import sys

from ds9481 import DS28E38, DS9481, Ds9481Error
from ds9481.errors import AdapterNotFound


def hexs(b):
    return " ".join("%02X" % x for x in b)


def step(n, text):
    print("[%d] %-34s" % (n, text), end=" ", flush=True)


def ok(msg=""):
    print("OK   " + msg)


def fail(msg):
    print("GAGAL  " + msg)
    return 1


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="COM port DS9481 (default: scan semua)")
    ap.add_argument("--debug", action="store_true")
    a = ap.parse_args()

    ad = DS9481(port=a.port, debug=a.debug)
    try:
        step(1, "Cari adaptor DS9481 (1-Wire)")
        try:
            ow = ad.onewire()
        except AdapterNotFound as e:
            return fail(str(e))
        ok("di %s" % ad.port)

        step(2, "Versi firmware adaptor")
        v = ad.version
        ok("%d.%d, I2C %s" % (v.firmware + ("didukung" if v.i2c_supported else "tidak didukung",)))

        step(3, "Presence pulse di bus 1-Wire")
        if not ow.reset():
            return fail("tidak ada perangkat 1-Wire. Cek kabel/konektor board DS28E38.")
        ok()

        chip = DS28E38(ad)

        step(4, "READ ROM + CRC8")
        rom = ow.read_rom()
        if rom is None:
            return fail("ROM tidak terbaca / CRC salah (lebih dari 1 perangkat di bus?)")
        chip.rom_no = rom
        ok(hexs(rom))

        step(5, "Read Status (XPC 0xAA)")
        try:
            st = chip.read_status()
        except Ds9481Error as e:
            return fail(str(e))
        chip.manid = bytes(st.manid)
        ok("MANID=%s versi=%s" % (hexs(st.manid), hexs(st.flavor)))
        for pg, prot in enumerate(st.protection):
            print("      proteksi halaman %d = 0x%02X" % (pg, prot))

        step(6, "Baca halaman 0-5")
        print()
        bad = 0
        for pg in range(6):
            try:
                print("      page %d  %s" % (pg, hexs(chip.read_memory(pg))))
            except Ds9481Error as e:
                bad += 1
                print("      page %d  GAGAL: %s" % (pg, e))

        step(7, "RNG 8 byte")
        try:
            ok(hexs(chip.read_rng(8)))
        except Ds9481Error as e:
            bad += 1
            print("GAGAL  %s" % e)

        print("\nHasil: %s" % ("SEMUA OK" if not bad else "%d langkah gagal" % bad))
        return 0 if not bad else 1
    except ImportError:
        print("pyserial belum ada: pip install pyserial")
        return 2
    finally:
        ad.close()


if __name__ == "__main__":
    sys.exit(main())
