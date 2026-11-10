#!/usr/bin/env python
"""Tanda tangan digital dengan DS28E38 lewat USB (DS9481P-300).

PAKAI:
  python sign.py info                              # lihat ID chip + kunci publik
  python sign.py sign "pesan yang mau ditandatangani"
  python sign.py sign --file dokumen.pdf           # tandatangani sebuah file
  python sign.py verify tanda_tangan.json          # cek tanda tangan (tanpa chip)
  python sign.py verify tanda_tangan.json "pesan"  # cek + pastikan pesannya sama

  Tambahkan --port COM13 jika port tidak ditemukan otomatis, --debug untuk
  melihat byte yang dikirim.

CARA KERJA
  Kunci privat ECDSA P-256 ada di dalam chip dan tidak pernah keluar. Pesan
  di-hash dengan SHA-256 dan hash itu dikirim ke chip sebagai "challenge" 32
  byte. Chip menandatangani SHA-256( ROM | isi halaman | challenge | no.halaman
  | MANID ) dan mengembalikan tanda tangan (r, s). Program ini lalu memeriksa
  tanda tangan itu di PC dengan kunci publik chip (halaman 4 dan 5).

  Perintah ini hanya MEMBACA dan menghitung tanda tangan. Tidak ada data yang
  ditulis ke chip, tidak ada yang dikunci.
"""

import argparse
import hashlib
import json
import sys
import time

from ds9481 import DS28E38, DS9481, Ds9481Error
from ds9481.ecdsa_verify import verify as ecdsa_verify

SIGN_PAGES = range(0, 6)   # halaman 6 = kunci privat (tidak bisa dibaca)


def hx(b):
    return bytes(b).hex()


def connect(args):
    ad = DS9481(port=args.port, debug=args.debug)
    chip = DS28E38(ad)
    print("Menghubungi chip lewat USB ...")
    chip.detect()
    print("  adaptor : %s (firmware %d.%d)" % ((ad.port,) + ad.version.firmware))
    print("  ROM ID  : %s" % hx(chip.rom_no).upper())
    print("  MANID   : %s" % hx(chip.manid).upper())
    return ad, chip


def read_public_key(chip):
    x = chip.read_memory(4)   # halaman 4 = Public Key X
    y = chip.read_memory(5)   # halaman 5 = Public Key Y
    return x, y


def get_message(args):
    if args.file:
        with open(args.file, "rb") as f:
            return f.read(), "file %s" % args.file
    if args.pesan is None:
        sys.exit("Tidak ada pesan. Tulis pesannya, mis.:  python sign.py sign \"halo\"   "
                 "atau  --file namafile")
    return args.pesan.encode("utf-8"), "teks %r" % args.pesan


def signed_digest(rom, page_data, challenge, page, manid):
    """SHA-256 dari pesan yang ditandatangani chip (lihat docstring)."""
    return hashlib.sha256(bytes(rom) + bytes(page_data) + bytes(challenge)
                          + bytes([page]) + bytes(manid)).digest()


def check(rec):
    """Verifikasi rekaman tanda tangan (dict) di PC. Mengembalikan True/False."""
    rom = b"\xFF" * 8 if rec["anonymous"] else bytes.fromhex(rec["rom"])
    digest = signed_digest(rom, bytes.fromhex(rec["page_data"]),
                           bytes.fromhex(rec["challenge"]), rec["page"],
                           bytes.fromhex(rec["manid"]))
    return ecdsa_verify(int(rec["pub_x"], 16), int(rec["pub_y"], 16), digest,
                        int(rec["r"], 16), int(rec["s"], 16))


# ------------------------------------------------------------------ perintah
def cmd_info(args):
    ad, chip = connect(args)
    try:
        x, y = read_public_key(chip)
        print("  Kunci publik X : %s" % hx(x).upper())
        print("  Kunci publik Y : %s" % hx(y).upper())
    finally:
        ad.close()
    return 0


def cmd_sign(args):
    if args.page not in SIGN_PAGES:
        sys.exit("--page harus 0..5 (halaman 6 menyimpan kunci privat)")
    message, desc = get_message(args)
    msg_hash = hashlib.sha256(message).digest()
    print("Pesan   : %s (%d byte)" % (desc, len(message)))
    print("SHA-256 : %s" % msg_hash.hex())

    ad, chip = connect(args)
    try:
        page_data = chip.read_memory(args.page)
        pub_x, pub_y = read_public_key(chip)
        print("Meminta chip menandatangani (halaman %d%s) ..." %
              (args.page, ", mode anonim" if args.anon else ""))
        t0 = time.time()
        r, s = chip.compute_read_page_auth(args.page, msg_hash, anonymous=args.anon)
        print("  selesai dalam %.2f detik" % (time.time() - t0))
    finally:
        ad.close()

    rec = {
        "format": "ds28e38-ecdsa-p256-v1",
        "message_sha256": msg_hash.hex(),
        "challenge": msg_hash.hex(),
        "page": args.page,
        "page_data": hx(page_data),
        "anonymous": bool(args.anon),
        "rom": hx(chip.rom_no),
        "manid": hx(chip.manid),
        "pub_x": hx(pub_x),
        "pub_y": hx(pub_y),
        "r": hx(r),
        "s": hx(s),
        "signature_rs": hx(r) + hx(s),
    }

    print("\nTanda tangan (r || s, 64 byte):")
    print("  r = %s" % rec["r"])
    print("  s = %s" % rec["s"])

    ok = check(rec)
    print("\nVerifikasi di PC dengan kunci publik chip : %s" %
          ("VALID" if ok else "TIDAK VALID"))
    if not ok:
        print("  Tanda tangan diterima dari chip tetapi tidak cocok dengan kunci publik di "
              "halaman 4/5.\n  Kemungkinan: halaman 4/5 bukan pasangan kunci privat chip "
              "(belum di-generate), atau urutan byte berbeda.")
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(rec, f, indent=2)
    print("Disimpan ke %s" % args.out)
    return 0 if ok else 3


def cmd_verify(args):
    with open(args.berkas, encoding="utf-8") as f:
        rec = json.load(f)
    if args.pesan is not None or args.file:
        message, desc = get_message(args)
        if hashlib.sha256(message).hexdigest() != rec["message_sha256"]:
            print("PESAN TIDAK SAMA dengan yang ditandatangani (%s)." % desc)
            return 4
        print("Pesan cocok dengan yang ditandatangani (%s)." % desc)
    else:
        print("(pesan tidak diberikan: hanya memeriksa tanda tangan atas hash %s...)"
              % rec["message_sha256"][:16])
    if args.pub:
        rec["pub_x"], rec["pub_y"] = [h.lower().replace("0x", "") for h in args.pub.split(",")]
        print("Memakai kunci publik dari --pub (bukan dari berkas).")
    else:
        print("PERINGATAN: kunci publik diambil dari berkas itu sendiri. Untuk keamanan "
              "nyata,\n  bandingkan dengan kunci publik tepercaya (--pub X,Y).")
    ok = check(rec)
    print("Tanda tangan : %s" % ("VALID" if ok else "TIDAK VALID"))
    return 0 if ok else 3


def main():
    # console Windows (cp1252) tidak bisa mencetak semua karakter pesan: jangan crash
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="replace")
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    def common(p):
        p.add_argument("--port", help="COM port DS9481, mis. COM13 (default: cari otomatis)")
        p.add_argument("--debug", action="store_true", help="tampilkan byte TX/RX")

    p = sub.add_parser("info", help="tampilkan ID chip dan kunci publik")
    common(p)
    p.set_defaults(fn=cmd_info)

    p = sub.add_parser("sign", help="tandatangani pesan atau file")
    p.add_argument("pesan", nargs="?", help="teks yang ditandatangani")
    p.add_argument("--file", help="tandatangani isi file ini")
    p.add_argument("--page", type=int, default=0, help="halaman memori yang diikat (0-5, default 0)")
    p.add_argument("--anon", action="store_true", help="mode anonim (ROM ID tidak ikut ditandatangani)")
    p.add_argument("--out", default="tanda_tangan.json", help="berkas hasil (default tanda_tangan.json)")
    common(p)
    p.set_defaults(fn=cmd_sign)

    p = sub.add_parser("verify", help="periksa tanda tangan (tanpa chip)")
    p.add_argument("berkas", help="berkas JSON hasil 'sign'")
    p.add_argument("pesan", nargs="?", help="pesan asli (opsional)")
    p.add_argument("--file", help="file asli (opsional)")
    p.add_argument("--pub", help="kunci publik tepercaya: X_hex,Y_hex")
    p.set_defaults(fn=cmd_verify)

    args = ap.parse_args()
    try:
        return args.fn(args)
    except Ds9481Error as e:
        print("\nGAGAL: %s" % e, file=sys.stderr)
        return 1
    except ImportError:
        print("pyserial belum ada: pip install pyserial", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
