#!/usr/bin/env python
"""Tes komunikasi KHUSUS DS2476 (I2C) lewat DS9481P-300. Hanya membaca.

  python test_ds2476.py --port COM13
  python test_ds2476.py --port COM13 --debug
  python test_ds2476.py --port COM13 --addr 0x76   (alamat 8-bit, default 0x76)
  python test_ds2476.py --port COM13 --no-scan

Langkah (berhenti di langkah pertama yang gagal):
  1 adaptor terdeteksi   2 adaptor mendukung I2C   3 pindah ke mode I2C
  4 pindai bus I2C       5 DS2476 menjawab di alamatnya (baca ROM options page)
  6 proteksi halaman     7 RNG
Adaptor dikembalikan ke mode 1-Wire saat selesai.
"""

import argparse
import sys

from ds9481 import DS2476, DS9481, Ds9481Error
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
    ap.add_argument("--addr", default="0x76", help="alamat I2C 8-bit (default 0x76)")
    ap.add_argument("--no-scan", action="store_true", help="lewati pemindaian alamat")
    ap.add_argument("--debug", action="store_true")
    a = ap.parse_args()
    addr = int(a.addr, 0)

    ad = DS9481(port=a.port, debug=a.debug)
    try:
        step(1, "Cari adaptor DS9481")
        try:
            ad.onewire()           # adaptor selalu mulai di mode 1-Wire
        except AdapterNotFound as e:
            return fail(str(e))
        ok("di %s" % ad.port)

        step(2, "Adaptor mendukung I2C")
        if not ad.version.i2c_supported:
            return fail("firmware adaptor %d.%d tanpa I2C" % ad.version.firmware)
        ok("firmware %d.%d" % ad.version.firmware)

        step(3, "Pindah ke mode I2C (+/- 5 detik)")
        try:
            bus = ad.i2c()
        except AdapterNotFound as e:
            return fail(str(e))
        ok("adaptor I2C versi byte 0x%s" % ("%02X" % (bus.read_version() or 0)))

        if not a.no_scan:
            step(4, "Pindai bus I2C 0x03-0x77")
            found = []
            for a7 in range(0x03, 0x78):
                bus.start()
                if bus.write_byte(a7 << 1):
                    found.append(a7)
                bus.stop()
            if not found:
                return fail("TIDAK ADA perangkat I2C yang menjawab. Cek: kabel SDA/SCL/GND ke "
                            "adaptor, catu daya (VCC) board DS2476, pull-up.")
            ok("ACK di alamat 7-bit %s (8-bit %s)" % (
                ", ".join("0x%02X" % x for x in found),
                ", ".join("0x%02X" % (x << 1) for x in found)))
            if (addr >> 1) not in found:
                print("      PERINGATAN: 0x%02X (7-bit 0x%02X) tidak menjawab. Coba --addr dengan "
                      "salah satu alamat 8-bit di atas." % (addr, addr >> 1))
        else:
            step(4, "Pindai bus I2C")
            print("dilewati")

        chip = DS2476(ad, addr=addr)
        step(5, "DS2476 menjawab di 0x%02X" % addr)
        if not chip.detect():
            return fail("tidak ada jawaban/ROM tidak valid (family 0x4E).")
        ok("ROM=%s MANID=%s" % (hexs(chip.rom_no), hexs(chip.manid)))

        bad = 0
        step(6, "Proteksi halaman")
        print()
        for pg in (0, 14, 15, 16, 17, 18, 19, 27, 29):
            try:
                print("      page %2d  proteksi 0x%02X" % (pg, chip.read_page_protection(pg)))
            except Ds9481Error as e:
                bad += 1
                print("      page %2d  GAGAL: %s" % (pg, e))

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
