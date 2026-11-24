"""Kalibrasi shunt INA226 dan offset nol (Bagian A1/A2 daftar kekurangan, 18 Nov 2026).

Skrip ini HANYA merekam lewat logger (perintah 7 = rekam saja) dan menghitung. Anda mengetik nilai acuan
(multimeter / resistor presisi) pada tiap titik. Hasil disimpan di data/kalibrasi/.

Alur:
  1. python tools/2_kalibrasi/kalibrasi_shunt.py offset --detik 60
         beban NOL: lepas IN- dari host (tegangan VBUS tetap ada). Hasil: offset (satuan 2,5 uV).
  2. python tools/2_kalibrasi/kalibrasi_shunt.py titik --nama stop --iref 1.98 [--vref 5.20] [--detik 30]
         ulangi untuk tiap titik arus tunak (lihat docs/prosedur_kalibrasi_bagian_A.md).
  3. python tools/2_kalibrasi/kalibrasi_shunt.py hitung [--u-ref 1.0]
         regresi: R_shunt efektif, offset hasil regresi, residu per titik, saran nilai --rshunt/--offset.
  Metode SELISIH (disarankan, 21 Nov 2026; offset terhapus, acuan = V_bus / R_nominal resistor 1 %):
  2'. python tools/2_kalibrasi/kalibrasi_shunt.py titik --kelompok terjaga --nama B1            (baseline, tanpa resistor)
      python tools/2_kalibrasi/kalibrasi_shunt.py titik --kelompok terjaga --nama R1k_1 --rload 1000
      ... urutan  B - 1k - B - 2k2 - B - ...  (baseline MENGAPIT tiap resistor), lalu ulangi untuk kelompok stop.
  3'. python tools/2_kalibrasi/kalibrasi_shunt.py selisih [--tol-r 1.0] [--u-v 0.5]
         R_shunt efektif per resistor/kelompok, uji kriteria penerimaan, u_kal, saran --rshunt/--offset.
Setiap perintah menerima --logger COMxx (bawaan COM18).

Konvensi (sama dengan tools/3_analisis/analisis_d.py): arus (mA) = (mentah - offset) x 2,5e-6 / R_shunt x 1000.
offset adalah rerata pembacaan mentah pada arus nol (negatif, mis. -3,2).
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import csv
import statistics
import sys
import time
from datetime import datetime
from pathlib import Path

import kalibrasi_param as kp  # noqa: E402
import run_bench as rb  # noqa: E402

OUT = ROOT / "data" / "kalibrasi"
LSB_V = kp.SHUNT_LSB_V
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


def _baca_csv(path):
    """Baca seluruh baris CSV sebagai daftar dict (berkas ditutup otomatis)."""
    with open(path, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def _sebaran_pct(v):
    """Sebaran maks-min relatif terhadap rerata, dalam persen."""
    return 100 * (max(v) - min(v)) / statistics.fmean(v)


def _arus_nominal_ma(mentah):
    """Arus (mA) bila shunt dianggap nominal (0,100 Ohm) dan mentah sudah bebas offset."""
    return mentah * LSB_V / kp.RSHUNT_NOMINAL * 1000


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
    print(f"  setara {r['shunt_mean'] * LSB_V * 1e6:+.2f} uV; pada shunt 0,100 Ohm = {_arus_nominal_ma(r['shunt_mean']):+.4f} mA")
    if r["bus_mean_V"] < 3:
        print("  PERINGATAN: tegangan bus < 3 V; VBUS modul tidak berdaya. Offset ini kurang mewakili kondisi ukur.")
    tulis(OUT / "offset.csv", {"waktu": datetime.now().isoformat(timespec="seconds"), "catatan": a.catatan, **r})
    print("tersimpan: data/kalibrasi/offset.csv. Analisis memakai offset = ", f"{r['shunt_mean']:.2f}", "(--offset)")


def cmd_titik(a):
    sh, bus = rekam(a.logger, a.detik)
    r = ringkas(sh, bus)
    if a.iref is None and a.kelompok is None:
        raise SystemExit("beri --iref (metode regresi) atau --kelompok (metode selisih)")
    print(f"TITIK {a.nama}: mentah {r['shunt_mean']:.2f} (sd {r['shunt_sd']:.2f}, n={r['n']}), bus {r['bus_mean_V']:.3f} V; "
          f"arus nominal (R=0,100, offset -2) = {_arus_nominal_ma(r['shunt_mean'] - kp.OFFSET_NOMINAL):.3f} mA; acuan = {a.iref} mA")
    if a.rload:
        print(f"  beban resistor {a.rload:.0f} Ohm: arus acuan V_bus/R = {r['bus_mean_V'] / a.rload * 1000:.3f} mA")
    tulis(OUT / "titik.csv", {"waktu": datetime.now().isoformat(timespec="seconds"), "nama": a.nama,
                              "iref_mA": a.iref if a.iref is not None else "",
                              "kelompok": a.kelompok or "", "rload_ohm": a.rload or 0,
                              "vref_V": a.vref if a.vref is not None else "", "catatan": a.catatan, **r})
    print("tersimpan: data/kalibrasi/titik.csv")


def _offset_terukur():
    fo = OUT / "offset.csv"
    if not fo.exists():
        return None, []
    nilai = [float(r["shunt_mean"]) for r in _baca_csv(fo)]
    return statistics.fmean(nilai), nilai


def hitung_selisih(rows, tol_r=1.0, u_v=0.5):
    """Metode selisih. rows = baris titik.csv berurutan waktu. Mengembalikan (hasil, peringatan).
    hasil: daftar dict per resistor ber-pengapit. R_eff = dV_shunt / dI, dI = V_bus / R_nominal."""
    hasil, peringatan = [], []
    per_kel = {}
    for r in rows:
        if r.get("kelompok"):
            per_kel.setdefault(r["kelompok"], []).append(r)
    for kel, rs in per_kel.items():
        for i, r in enumerate(rs):
            rl = float(r["rload_ohm"] or 0)
            if rl <= 0:
                continue
            # baseline pengapit = tetangga langsung (sebelum/sesudah) yang tanpa resistor
            tetangga = [rs[j] for j in (i - 1, i + 1) if 0 <= j < len(rs) and float(rs[j]["rload_ohm"] or 0) <= 0]
            if not tetangga:
                peringatan.append(f"{kel}/{r['nama']}: tanpa baseline pengapit, dilewati")
                continue
            if len(tetangga) == 1:
                peringatan.append(f"{kel}/{r['nama']}: hanya satu baseline pengapit (drift tidak terhapus)")
            xb = statistics.fmean(float(t["shunt_mean"]) for t in tetangga)
            # drift = beda dua baseline pengapit (0 bila hanya satu)
            drift = (float(tetangga[0]["shunt_mean"]) - float(tetangga[-1]["shunt_mean"])) if len(tetangga) == 2 else 0.0
            dx = float(r["shunt_mean"]) - xb
            if dx <= 0:
                peringatan.append(f"{kel}/{r['nama']}: selisih shunt <= 0, dilewati")
                continue
            i_ref = float(r["bus_mean_V"]) / rl * 1000.0
            hasil.append({"kelompok": kel, "nama": r["nama"], "rload": rl, "dx": dx, "drift": drift, "i_ref": i_ref,
                          "r_eff": LSB_V * dx / (i_ref / 1000.0),
                          "vref": float(r["vref_V"]) if r.get("vref_V") else None, "vbus": float(r["bus_mean_V"])})
    return hasil, peringatan


def _saring_hanya_r(hasil, peringatan, hanya_r):
    """Pakai hanya resistor bernilai `hanya_r` (ohm); sisanya dilaporkan sebagai dikeluarkan."""
    dikeluarkan = [h for h in hasil if h["rload"] not in hanya_r]
    hasil = [h for h in hasil if h["rload"] in hanya_r]
    # peringatan berbentuk "kelompok/nama: ..." dengan nama titik "R1k_1" / "B1": cocokkan lewat teksnya
    peringatan = [p for p in peringatan if any(f"/R{int(r / 1000)}k" in p for r in hanya_r) or "/B" in p]
    print(f"CATATAN: penentuan R_eff HANYA memakai resistor {sorted(hanya_r)} ohm; "
          f"{len(dikeluarkan)} selisih dikeluarkan (keputusan setelah data diamati): "
          + ", ".join(f"{h['nama']}={h['r_eff']:.5f}" for h in dikeluarkan))
    return hasil, peringatan


def _cetak_tabel(hasil):
    print(f"{'kelompok':9s} {'titik':9s} {'R(ohm)':>7s} {'dX':>9s} {'drift B':>8s} {'dI ref mA':>10s} {'R_eff':>9s}")
    for h in hasil:
        print(f"{h['kelompok']:9s} {h['nama']:9s} {h['rload']:7.0f} {h['dx']:9.2f} {h['drift']:8.2f} {h['i_ref']:10.3f} {h['r_eff']:9.5f}")
        if h["vref"]:
            print(f"{'':9s} silang V_bus vs DCV multimeter: {100 * (h['vbus'] / h['vref'] - 1):+.2f} %")


def _periksa_kriteria(grup):
    """Kriteria penerimaan metode selisih. grup = {(kelompok, resistor): [R_eff, ...]}. True bila lulus."""
    gagal = []
    # 1: sebaran R_eff berulang pada resistor yang sama < 1 %
    for (kel, rl), v in grup.items():
        if len(v) > 1 and _sebaran_pct(v) >= 1.0:
            gagal.append(f"  GAGAL kriteria 1: sebaran {kel}/{rl:.0f} ohm >= 1 %")
    # 2: rerata R_eff antar resistor dalam satu kelompok berbeda < 2 %
    for kel in {k for k, _ in grup}:
        rata = [statistics.fmean(v) for (k, _), v in grup.items() if k == kel]
        if len(rata) > 1 and _sebaran_pct(rata) >= 2.0:
            gagal.append(f"  GAGAL kriteria 2: R_eff antar resistor di kelompok {kel} berbeda >= 2 %")
    # 3: rerata R_eff antar kelompok (terjaga vs stop) berbeda < 2 %
    per_kelompok = {}
    for (kel, _), v in grup.items():
        per_kelompok.setdefault(kel, []).extend(v)
    rata_kel = [statistics.fmean(v) for v in per_kelompok.values()]
    if len(rata_kel) > 1 and _sebaran_pct(rata_kel) >= 2.0:
        gagal.append("  GAGAL kriteria 3: R_eff antar kelompok (terjaga vs stop) berbeda >= 2 %")
    for g in gagal:
        print(g)
    return not gagal


def _cetak_saran(rshunt, offset_teks, u_kal_pct):
    print("\nSaran penggunaan:")
    print(f"  python tools/3_analisis/analisis_d.py --manifest data/tahap_stop.txt --stop --rshunt {rshunt:.5f} --offset {offset_teks} "
          f"--u-rel {u_kal_pct / 100:.3f} --keluar data/analisis/stop_final")
    print(f"  python tools/3_analisis/gambar_bab4_stop.py --shunt {rshunt:.5f}")


def cmd_selisih(a):
    f = OUT / "titik.csv"
    if not f.exists():
        raise SystemExit("belum ada titik.csv; jalankan perintah 'titik' dulu")
    hasil, peringatan = hitung_selisih(_baca_csv(f), a.tol_r, a.u_v)
    if a.hanya_r:
        hasil, peringatan = _saring_hanya_r(hasil, peringatan, a.hanya_r)
    for p in peringatan:
        print("PERINGATAN:", p)
    if not hasil:
        raise SystemExit("tidak ada resistor ber-baseline; periksa urutan B - R - B dan kolom kelompok/rload_ohm")
    _cetak_tabel(hasil)

    # statistik per (kelompok, resistor)
    grup = {}
    for h in hasil:
        grup.setdefault((h["kelompok"], h["rload"]), []).append(h["r_eff"])
    print("\nPer resistor:")
    for (kel, rl), v in sorted(grup.items()):
        sebaran = _sebaran_pct(v) if len(v) > 1 else float("nan")
        print(f"  {kel:9s} {rl:6.0f} ohm: R_eff = {statistics.fmean(v):.5f} (n={len(v)}, sebaran maks-min = {sebaran:.2f} %)")

    # R_eff akhir = rerata semua selisih; u_kal = akar(toleransi R^2 + u V_bus^2 + (sebaran/akar n)^2)
    semua = [h["r_eff"] for h in hasil]
    r_eff = statistics.fmean(semua)
    sd_rel = 100 * statistics.pstdev(semua) / r_eff if len(semua) > 1 else 0.0
    u_fit = sd_rel / len(semua) ** 0.5
    u_kal = (a.tol_r ** 2 + a.u_v ** 2 + u_fit ** 2) ** 0.5
    print(f"\nR_shunt efektif (rerata {len(semua)} selisih) = {r_eff:.5f} Ohm; faktor koreksi arus = {kp.RSHUNT_NOMINAL / r_eff:.4f}")
    print(f"sebaran sd = {sd_rel:.2f} %; u_kal ~ {u_kal:.2f} % (toleransi R {a.tol_r:.1f} %, V_bus {a.u_v:.1f} %, sebaran {u_fit:.2f} %)")

    ok = _periksa_kriteria(grup)
    print("  Kriteria penerimaan:", "LULUS" if ok else "TIDAK LULUS (tambah putaran hanya di kondisi yang gagal)")
    off, nilai = _offset_terukur()
    if off is None:
        print("  offset.csv belum ada: jalankan 'offset' (A2); selisih tidak memerlukannya, tetapi analisis nilai mutlak memerlukannya.")
        off_s = "<offset>"
    else:
        print(f"  offset terukur: {', '.join(f'{n:.2f}' for n in nilai)} (rerata {off:.2f} satuan)")
        off_s = f"{off:.2f}"
    _cetak_saran(r_eff, off_s, u_kal)


def cmd_hitung(a):
    f = OUT / "titik.csv"
    fo = OUT / "offset.csv"
    if not f.exists():
        raise SystemExit("belum ada titik.csv; jalankan perintah 'titik' dulu")
    rows = [r for r in _baca_csv(f) if r.get("iref_mA") not in ("", None)]
    if not rows:
        raise SystemExit("tidak ada titik ber-iref; untuk metode selisih pakai 'selisih'")
    off = None
    if fo.exists():
        off = float(_baca_csv(fo)[-1]["shunt_mean"])   # regresi memakai offset TERAKHIR (selisih: rerata semua)
    if a.offset is not None:
        off = a.offset
    if off is None:
        raise SystemExit("offset belum ada: jalankan 'offset' atau beri --offset")
    x = [float(r["shunt_mean"]) - off for r in rows]          # satuan 2,5 uV bersih
    y = [float(r["iref_mA"]) for r in rows]
    nama = [r["nama"] for r in rows]
    # regresi melalui titik nol: y = k x  (k dalam mA per satuan)
    k = sum(xx * yy for xx, yy in zip(x, y)) / sum(xx * xx for xx in x)
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
        print(f"{nm:14s} {yy:8.3f} {_arus_nominal_ma(xx):13.3f} {xx * k:13.3f} {yy / _arus_nominal_ma(xx):18.4f} {rr:9.2f}")
    sd_rel = statistics.pstdev(res_rel) if n > 1 else 0.0
    u_fit = (sd_rel / (n ** 0.5)) if n > 1 else 0.0
    u_kal = (a.u_ref ** 2 + u_fit ** 2) ** 0.5
    print(f"\nR_shunt efektif (melalui nol) = {r_eff:.5f} Ohm  (nominal 0,100; faktor koreksi arus = {k / _arus_nominal_ma(1):.4f})")
    print(f"regresi bebas: kemiringan -> R = {LSB_V * 1000 / b:.5f} Ohm, intersep arus = {c:+.4f} mA "
          f"(setara {c / b:+.2f} satuan offset tambahan)")
    print(f"sebaran residu relatif sd = {sd_rel:.2f} %; ketidakpastian acuan = {a.u_ref:.2f} %; u_kal ~ {u_kal:.2f} % (tanpa asumsi 5 %)")
    _cetak_saran(r_eff, f"{off:.2f}", u_kal)
    if abs(c) > 0.05 * max(y):
        print("  CATATAN: intersep besar; periksa titik nol/offset atau nonlinearitas sebelum memakai R_eff.")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sp = ap.add_subparsers(dest="aksi", required=True)
    for nm in ("offset", "titik", "hitung", "selisih"):
        p = sp.add_parser(nm)
        if nm != "selisih":
            p.add_argument("--logger", default="COM18")
            p.add_argument("--catatan", default="")
        if nm in ("offset", "titik"):
            p.add_argument("--detik", type=int, default=60 if nm == "offset" else 30)
        if nm == "titik":
            p.add_argument("--nama", required=True)
            p.add_argument("--iref", type=float, default=None, help="arus acuan (mA) dari multimeter (metode regresi)")
            p.add_argument("--vref", type=float, default=None, help="tegangan acuan (V), opsional (pemeriksaan silang V_bus)")
            p.add_argument("--kelompok", default=None, help="metode selisih: terjaga | stop")
            p.add_argument("--rload", type=float, default=None, help="metode selisih: nilai NOMINAL resistor beban (Ohm); kosong = baseline")
        if nm == "hitung":
            p.add_argument("--u-ref", type=float, default=1.0, help="ketidakpastian relatif acuan (%%), dari spesifikasi multimeter")
            p.add_argument("--offset", type=float, default=None)
        if nm == "selisih":
            p.add_argument("--tol-r", type=float, default=1.0, help="toleransi resistor (%%)")
            p.add_argument("--u-v", type=float, default=0.5, help="ketidakpastian V_bus INA226 (%%); konservatif, cek datasheet TI")
            p.add_argument("--hanya-r", type=float, nargs="+", default=None,
                           help="hanya pakai resistor bernilai ini (ohm) untuk R_eff; sisanya dilaporkan sebagai dikeluarkan")
    a = ap.parse_args()
    {"offset": cmd_offset, "titik": cmd_titik, "hitung": cmd_hitung, "selisih": cmd_selisih}[a.aksi](a)


if __name__ == "__main__":
    main()
