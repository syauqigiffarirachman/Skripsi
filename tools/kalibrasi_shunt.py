"""Kalibrasi shunt INA226 dan offset nol (Bagian A1/A2 daftar kekurangan, 18 Nov 2026).

Skrip ini HANYA merekam lewat logger (perintah 7 = rekam saja) dan menghitung. Anda mengetik nilai acuan
(multimeter / resistor presisi) pada tiap titik. Hasil disimpan di data/kalibrasi/.

Alur:
  1. python tools/kalibrasi_shunt.py offset --detik 60
         beban NOL: lepas IN- dari host (tegangan VBUS tetap ada). Hasil: offset (satuan 2,5 uV).
  2. python tools/kalibrasi_shunt.py titik --nama stop --iref 1.98 [--vref 5.20] [--detik 30]
         ulangi untuk tiap titik arus tunak (lihat docs/prosedur_kalibrasi_bagian_A.md).
  3. python tools/kalibrasi_shunt.py hitung [--u-ref 1.0]
         regresi: R_shunt efektif, offset hasil regresi, residu per titik, saran nilai --rshunt/--offset.
Setiap perintah menerima --logger COMxx (bawaan COM18).

Konvensi (sama dengan tools/analisis_d.py): arus (mA) = (mentah - offset) x 2,5e-6 / R_shunt x 1000.
offset adalah rerata pembacaan mentah pada arus nol (negatif, mis. -3,2).
"""
import argparse
import csv
import statistics
import sys
import time
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import run_bench as rb  # noqa: E402

OUT = Path(__file__).resolve().parent.parent / "data" / "kalibrasi"
LSB_V = 2.5e-6
LSB_BUS = 1.25e-3


def rekam(port, detik):
    lg = rb.Logger(port)
    try:
        raw = lg.command(7, detik, detik + 30).decode(errors="replace")
    finally:
        lg.close()
    sh, bus = [], []
    for ln in raw.splitlines():
        if ln.startswith("s,"):
            p = ln.split(",")
            if len(p) == 4:
                sh.append(int(p[2]))
                bus.append(int(p[3]))
    if len(sh) < 50:
        raise SystemExit(f"rekaman terlalu pendek ({len(sh)} cuplikan); periksa logger/kabel")
    return sh, bus


def ringkas(sh, bus):
    n = len(sh)
    m = statistics.fmean(sh)
    sd = statistics.pstdev(sh)
    return {"n": n, "shunt_mean": m, "shunt_sd": sd, "shunt_sem": sd / n ** 0.5,
            "bus_mean_V": statistics.fmean(bus) * LSB_BUS, "shunt_min": min(sh), "shunt_max": max(sh)}


def tulis(path, baris):
    OUT.mkdir(parents=True, exist_ok=True)
    baru = not path.exists()
    with open(path, "a", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(baris.keys()))
        if baru:
            w.writeheader()
        w.writerow(baris)


def cmd_offset(a):
    sh, bus = rekam(a.logger, a.detik)
    r = ringkas(sh, bus)
    print(f"OFFSET (beban nol): rerata mentah = {r['shunt_mean']:.3f} satuan (sd {r['shunt_sd']:.3f}, "
          f"sem {r['shunt_sem']:.4f}, n={r['n']}, min {r['shunt_min']}, maks {r['shunt_max']}); "
          f"tegangan bus {r['bus_mean_V']:.3f} V")
    print(f"  setara {r['shunt_mean'] * LSB_V * 1e6:+.2f} uV; pada shunt 0,100 Ohm = {r['shunt_mean'] * LSB_V / 0.1 * 1000:+.4f} mA")
    if r["bus_mean_V"] < 3:
        print("  PERINGATAN: tegangan bus < 3 V; VBUS modul tidak berdaya. Offset ini kurang mewakili kondisi ukur.")
    tulis(OUT / "offset.csv", {"waktu": datetime.now().isoformat(timespec="seconds"), "catatan": a.catatan, **r})
    print("tersimpan: data/kalibrasi/offset.csv. Analisis memakai offset = ", f"{r['shunt_mean']:.2f}", "(--offset)")


def cmd_titik(a):
    sh, bus = rekam(a.logger, a.detik)
    r = ringkas(sh, bus)
    print(f"TITIK {a.nama}: mentah {r['shunt_mean']:.2f} (sd {r['shunt_sd']:.2f}, n={r['n']}), bus {r['bus_mean_V']:.3f} V; "
          f"arus nominal (R=0,100, offset -2) = {(r['shunt_mean'] + 2) * LSB_V / 0.1 * 1000:.3f} mA; acuan = {a.iref} mA")
    tulis(OUT / "titik.csv", {"waktu": datetime.now().isoformat(timespec="seconds"), "nama": a.nama, "iref_mA": a.iref,
                              "vref_V": a.vref if a.vref is not None else "", "catatan": a.catatan, **r})
    print("tersimpan: data/kalibrasi/titik.csv")


def cmd_hitung(a):
    f = OUT / "titik.csv"
    fo = OUT / "offset.csv"
    if not f.exists():
        raise SystemExit("belum ada titik.csv; jalankan perintah 'titik' dulu")
    rows = list(csv.DictReader(open(f, encoding="utf-8")))
    off = None
    if fo.exists():
        orow = list(csv.DictReader(open(fo, encoding="utf-8")))
        off = float(orow[-1]["shunt_mean"])
    if a.offset is not None:
        off = a.offset
    if off is None:
        raise SystemExit("offset belum ada: jalankan 'offset' atau beri --offset")
    x = [float(r["shunt_mean"]) - off for r in rows]          # satuan 2,5 uV bersih
    y = [float(r["iref_mA"]) for r in rows]
    nama = [r["nama"] for r in rows]
    # regresi melalui titik nol: y = k x  (k dalam mA per satuan)
    k = sum(a_ * b for a_, b in zip(x, y)) / sum(a_ * a_ for a_ in x)
    r_eff = LSB_V * 1000.0 / k
    res = [(yy - k * xx) for xx, yy in zip(x, y)]
    res_rel = [100.0 * rr / yy for rr, yy in zip(res, y)]
    n = len(x)
    # regresi bebas (kemiringan + intersep) sebagai pemeriksaan offset
    mx, my = statistics.fmean(x), statistics.fmean(y)
    sxx = sum((xx - mx) ** 2 for xx in x)
    b = sum((xx - mx) * (yy - my) for xx, yy in zip(x, y)) / sxx if sxx else float("nan")
    c = my - b * mx
    print(f"Offset dipakai: {off:.3f} satuan  ({n} titik)")
    print(f"{'titik':14s} {'I_ref':>8s} {'I_ina(0,100)':>13s} {'I_ina(R_eff)':>13s} {'rasio ref/nominal':>18s} {'residu %':>9s}")
    for nm, xx, yy, rr in zip(nama, x, y, res_rel):
        print(f"{nm:14s} {yy:8.3f} {xx * LSB_V / 0.1 * 1000:13.3f} {xx * k:13.3f} {yy / (xx * LSB_V / 0.1 * 1000):18.4f} {rr:9.2f}")
    sd_rel = statistics.pstdev(res_rel) if n > 1 else 0.0
    u_fit = (sd_rel / (n ** 0.5)) if n > 1 else 0.0
    u_kal = (a.u_ref ** 2 + u_fit ** 2) ** 0.5
    print(f"\nR_shunt efektif (melalui nol) = {r_eff:.5f} Ohm  (nominal 0,100; faktor koreksi arus = {k / (LSB_V * 1000 / 0.1):.4f})")
    print(f"regresi bebas: kemiringan -> R = {LSB_V * 1000 / b:.5f} Ohm, intersep arus = {c:+.4f} mA "
          f"(setara {c / b:+.2f} satuan offset tambahan)")
    print(f"sebaran residu relatif sd = {sd_rel:.2f} %; ketidakpastian acuan = {a.u_ref:.2f} %; u_kal ~ {u_kal:.2f} % (tanpa asumsi 5 %)")
    print("\nSaran penggunaan:")
    print(f"  python tools/analisis_d.py --manifest data/tahap_stop.txt --stop --rshunt {r_eff:.5f} --offset {off:.2f} "
          f"--u-rel {u_kal / 100:.3f} --keluar data/analisis/stop_final")
    print(f"  python tools/gambar_bab4_stop.py --shunt {r_eff:.5f}")
    if abs(c) > 0.05 * max(y):
        print("  CATATAN: intersep besar; periksa titik nol/offset atau nonlinearitas sebelum memakai R_eff.")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sp = ap.add_subparsers(dest="aksi", required=True)
    for nm in ("offset", "titik", "hitung"):
        p = sp.add_parser(nm)
        p.add_argument("--logger", default="COM18")
        p.add_argument("--catatan", default="")
        if nm in ("offset", "titik"):
            p.add_argument("--detik", type=int, default=60 if nm == "offset" else 30)
        if nm == "titik":
            p.add_argument("--nama", required=True)
            p.add_argument("--iref", type=float, required=True, help="arus acuan (mA) dari multimeter / hitungan")
            p.add_argument("--vref", type=float, default=None, help="tegangan acuan (V), opsional")
        if nm == "hitung":
            p.add_argument("--u-ref", type=float, default=1.0, help="ketidakpastian relatif acuan (%%), dari spesifikasi multimeter")
            p.add_argument("--offset", type=float, default=None)
    a = ap.parse_args()
    {"offset": cmd_offset, "titik": cmd_titik, "hitung": cmd_hitung}[a.aksi](a)


if __name__ == "__main__":
    main()
