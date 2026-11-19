"""B1: verifikasi di komputer tanda tangan yang dihasilkan firmware Stop (K1S, K2S) pada jalur layanan sebenarnya.

Firmware diagnostik (env host_k1_stop_diag_auto / host_k2_stop_diag_auto, flag -DDIAG_SIG) menyimpan r||s (K1S) atau
keluaran mentah chip 64 B (K2S) untuk 30 permintaan pertama, lalu mengirimnya sebagai baris 'h,...,>SIG,<i>,<hex128>'
SEBELUM '>DONE' (K2S juga '>ROM,<hex16>'). Alur permintaan, Stop, dan penerima frame identik dengan firmware final
(flag hanya menambah penyalinan hasil dan pengiriman baris).

Pemeriksaan (semua harus lulus):
  K1S: kunci publik = d x G (d dari firmware sumber), tiap r||s lolos verifikasi ECDSA P-256 terhadap DIGEST,
       s xor 1 ditolak, semua r berbeda (nonce hedged).
  K2S: ROM firmware = ROM chip pada rekaman pengembangan ds_od; tiap tanda tangan lolos terhadap
       pesan = SHA-256(ROM || page0 || DIGEST || 0x00 || MANID) dengan kunci publik chip; s xor 1 ditolak.

Pakai:  python tools/b1_verifikasi.py K1S|K2S [--secs 60] [--logger COM18 --attacker COM22]   (ambil data + verifikasi)
        python tools/b1_verifikasi.py K1S|K2S --analyse data/dev/b1/<berkas>.csv               (hanya verifikasi)
Keluaran: data/dev/b1/<waktu>_<arch>_sig.csv (rekaman mentah) dan ringkasan LULUS/GAGAL.
"""
import argparse
import hashlib
import sys
import time
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from p256 import G, mul, n, on_curve, verify  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "data" / "dev" / "b1"
DIGEST = bytes.fromhex("D72C103978443C67EAD7542910F0EBF1C8BAF63182C07E062BCC6FC2E9A8A50E")
D_K1 = bytes.fromhex("643EC908DC6AF993A3287913" "13241E693FAAD7A2334208F12C99021AD3E0223D")
CHIP_REKAMAN = ROOT / "data" / "dev" / "ds_od" / "20261117_234844_ds_od_pertama.csv"   # INFO ROM/MANID/P0/PX/PY chip
GAP_S, CAL_S, KIRIM_S = 8, 2, 6


def baca_sig(text):
    """-> (daftar sig 64 B, rom hex atau None) dari baris h yang memuat >SIG / >ROM."""
    sig, rom = {}, None
    for ln in text.splitlines():
        if not ln.startswith("h,"):
            continue
        f = ln.strip().split(",")
        if len(f) >= 5 and f[2] == ">SIG" and len(f[4]) == 128:
            sig[int(f[3])] = bytes.fromhex(f[4])
        elif len(f) >= 4 and f[2] == ">ROM":
            rom = f[3].lower()
    return [sig[k] for k in sorted(sig)], rom


def info_chip():
    info = {}
    for ln in CHIP_REKAMAN.read_text(errors="replace").splitlines():
        f = ln.split(",")
        if ln.startswith("h,") and len(f) >= 5 and f[2] == "INFO":
            info[f[3]] = f[4]
    return info


def verifikasi(arch, text):
    sigs, rom = baca_sig(text)
    gagal = []

    def cek(ok, pesan):
        print(("  [OK]    " if ok else "  [GAGAL] ") + pesan)
        if not ok:
            gagal.append(pesan)

    cek(len(sigs) >= 30, f"jumlah tanda tangan diterima = {len(sigs)} (minimal 30)")
    if arch == "K1S":
        pub = mul(int.from_bytes(D_K1, "big"), G)
        cek(on_curve(pub), "kunci publik d x G pada kurva")
        pesan, urut = DIGEST, lambda b: (int.from_bytes(b[:32], "big"), int.from_bytes(b[32:], "big"))   # r||s
    else:
        info = info_chip()
        cek(rom is not None and rom == info["ROM"].lower(), f"ROM dari firmware = ROM chip rekaman pengembangan ({rom})")
        pub = (int.from_bytes(bytes.fromhex(info["PX"]), "big"), int.from_bytes(bytes.fromhex(info["PY"]), "big"))
        cek(on_curve(pub), "kunci publik chip pada kurva")
        pesan = hashlib.sha256(bytes.fromhex(info["ROM"])[:8] + bytes.fromhex(info["P0"]) + DIGEST + b"\x00"
                               + bytes.fromhex(info["MANID"])).digest()
        urut = lambda b: (int.from_bytes(b[32:], "big"), int.from_bytes(b[:32], "big"))                  # chip: s||r
    if arch == "K2S":                                   # urutan keluaran chip: s||r (eksperimen ds_od); coba juga r||s bila s||r gagal semua
        alt = lambda b: (int.from_bytes(b[:32], "big"), int.from_bytes(b[32:], "big"))
        if sigs and not any(verify(pub, pesan, *urut(b)) for b in sigs) and any(verify(pub, pesan, *alt(b)) for b in sigs):
            print("  [CATATAN] urutan byte keluaran chip ternyata r||s, bukan s||r; memakai urutan itu")
            urut = alt
    valid = tolak = 0
    rs = []
    for b in sigs:
        r, s = urut(b)
        rs.append(r)
        valid += verify(pub, pesan, r, s)
        tolak += not verify(pub, pesan, r, s ^ 1)
    cek(valid == len(sigs) and sigs, f"{valid}/{len(sigs)} tanda tangan sah menurut verifikasi ECDSA P-256 di komputer")
    cek(tolak == len(sigs) and sigs, f"{tolak}/{len(sigs)} tanda tangan yang dirusak (s xor 1) ditolak")
    if arch == "K1S":
        cek(len(set(rs)) == len(rs), f"semua r berbeda ({len(set(rs))}/{len(rs)}): nonce hedged")
    print("HASIL:", "LULUS" if not gagal else f"GAGAL ({len(gagal)} butir)")
    return not gagal


def ambil(arch, a):
    import run_bench as rb
    OUT.mkdir(parents=True, exist_ok=True)
    at = rb.Attacker(a.attacker)
    lg = rb.Logger(a.logger)

    def on_line(line):
        if line.startswith(b"h,") and b">START" in line:
            at.send("a")
        elif line.startswith(b"h,") and b">END" in line:
            at.send("x")

    try:
        at.stop()
        siklus = GAP_S + a.secs + CAL_S + KIRIM_S
        lg.command(7, siklus + 25, siklus + 55)                        # sinkron: rekaman berakhir sesudah satu siklus
        raw = lg.command(7, siklus + 20, siklus + 50, on_line)
    finally:
        try:
            at.stop()
        except Exception as e:  # noqa: BLE001
            print("peringatan: attacker tidak berhenti:", e)
        lg.close()
        at.close()
    f = OUT / f"{datetime.now():%Y%m%d_%H%M%S}_{arch}_sig.csv"
    f.write_bytes(raw)
    print("rekaman:", f)
    return raw.decode(errors="replace")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("arch", choices=["K1S", "K2S"])
    ap.add_argument("--secs", type=int, default=60)
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--attacker", default="COM24")
    ap.add_argument("--analyse", metavar="CSV")
    a = ap.parse_args()
    text = Path(a.analyse).read_text(errors="replace") if a.analyse else ambil(a.arch, a)
    sys.exit(0 if verifikasi(a.arch, text) else 1)


if __name__ == "__main__":
    main()
