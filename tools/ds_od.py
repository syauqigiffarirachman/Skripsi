"""Uji DS28E38: overdrive + tunggu sign berbasis polling (17 Nov 2026), firmware env host_ds_od.

Merekam baris 'h,' dari host lewat logger (perintah 7, rekam saja) selama --detik (default 40 s = >= 1 siklus
firmware), lalu mencetak ringkasan dan MEMVERIFIKASI tiap tanda tangan di PC:
  pesan = SHA-256(ROM || page0 || challenge || 0x00 || MANID), r = chip[32:64], s = chip[0:32] (seperti k2_check.py)
  + tanda tangan dirusak (s xor 1) harus ditolak.

Pakai: python tools/ds_od.py [--detik 40] [--logger COM18] [--catatan x]
       python tools/ds_od.py --analyse data/dev/ds_od/<berkas>.csv
"""
import argparse
import hashlib
import statistics
import sys
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from p256 import on_curve, verify  # noqa: E402
from run_bench import Logger  # noqa: E402

OUT = Path(__file__).resolve().parent.parent / "data" / "dev" / "ds_od"
DIGEST = bytes.fromhex("D72C103978443C67EAD7542910F0EBF1C8BAF63182C07E062BCC6FC2E9A8A50E")


def analyse(text):
    info, signs, odcfg, rc, selesai = {}, [], [], [], 0
    for ln in text.splitlines():
        if not ln.startswith("h,"):
            continue
        f = ln.split(",")[2:]
        if not f:
            continue
        if f[0] == "RC":
            rc.append(int(f[1]))
        elif f[0] == "INFO":
            info[f[1]] = f[2]
        elif f[0] == "SIGN" and len(f) >= 8:
            signs.append(f[1:8])
        elif f[0] == "ODCFG":
            odcfg.append(f[1:])
        elif f[0] == "SIKLUS":
            selesai += 1
    print(f"siklus selesai: {selesai}; waktu naik bus (ns): {rc}")
    print("INFO:", {k: (v[:20] + '...' if len(v) > 20 else v) for k, v in info.items()})
    print("Sapuan OD (i, A_ns, E_ns, H_ns, masuk, ok/3):")
    seen = set()
    for c in odcfg:
        key = tuple(c)
        if key in seen:
            continue
        seen.add(key)
        print("   ", c)
    try:
        rom, manid, p0 = (bytes.fromhex(info[k]) for k in ("ROM", "MANID", "P0"))
        px, py = bytes.fromhex(info["PX"]), bytes.fromhex(info["PY"])
        pub = (int.from_bytes(px, "big"), int.from_bytes(py, "big"))
        pesan = hashlib.sha256(rom[:8] + p0 + DIGEST + b"\x00" + manid).digest()
        kunci_ok = on_curve(pub)
    except (KeyError, ValueError):
        pesan, pub, kunci_ok = None, None, False
    print("kunci publik di kurva:", kunci_ok)
    by = {}
    for mode, kirim, sibuk, baca, total, err, hx in signs:
        sah = None
        if err == "0" and pesan and len(hx) == 128:
            s_, r_ = int(hx[:64], 16), int(hx[64:], 16)
            sah = verify(pub, pesan, r_, s_) and not verify(pub, pesan, r_, s_ ^ 1)
        by.setdefault(mode, []).append((int(kirim), int(sibuk), int(baca), int(total), err, sah))
    print("\nRingkasan tanda tangan (us) per mode:")
    for mode, rows in by.items():
        okr = [r for r in rows if r[4] == "0"]
        valid = sum(1 for r in okr if r[5])
        if okr:
            m = lambda i: statistics.fmean(r[i] for r in okr)  # noqa: E731
            print(f"  {mode:8s} n={len(rows)} err={len(rows) - len(okr)} valid_PC={valid}/{len(okr)}  "
                  f"kirim={m(0):.0f} sibuk={m(1):.0f} baca={m(2):.0f} TOTAL={m(3):.0f} us ({m(3)/1000:.1f} ms)")
        else:
            print(f"  {mode:8s} n={len(rows)} semua galat: {sorted(set(r[4] for r in rows))}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--detik", type=int, default=40)
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--catatan", default="")
    ap.add_argument("--analyse", metavar="CSV")
    a = ap.parse_args()
    if a.analyse:
        analyse(Path(a.analyse).read_text(errors="replace"))
        return
    OUT.mkdir(parents=True, exist_ok=True)
    lg = Logger(a.logger)
    try:
        raw = lg.command(7, a.detik, a.detik + 25)
    finally:
        lg.close()
    f = OUT / f"{datetime.now():%Y%m%d_%H%M%S}_ds_od_{a.catatan or 'run'}.csv"
    f.write_bytes(raw)
    print("log:", f)
    analyse(raw.decode(errors="replace"))


if __name__ == "__main__":
    main()
