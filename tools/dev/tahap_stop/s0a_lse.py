"""Deep sleep langkah 4 / S0a: uji kristal LSE host terhadap kristal logger (17 Nov 2026).

Host menjalankan env host_s0a_lse: RTC dari LSE membalik PB1 tiap 1/8 s (kotak 4 Hz, periode
250 ms) dan mengirim status "S0A,<tik>,<lse_ok>,<lse_start_ms>,<rtc_cnt>,<bdcr>" tiap 1 s lewat
USART1. Skrip ini meminta logger merekam saja (perintah 7, tanpa jendela host) selama <detik>, lalu:
  - frekuensi LSE relatif ke kristal logger 72 MHz (ppm) dari periode tepi naik PB1;
  - sebaran periode (jitter ISR) dan tepi hilang/salah urut;
  - status host (lse_ok, waktu start LSE).
Satu baris hasil ditambahkan ke data/dev/s0a/ringkasan.csv; log mentah disimpan di folder yang sama.

Uji lulus langkah 4: lse_ok=1 di >= 10 power-cycle host, |ppm| <= 50 (gabungan +-20 ppm MC-306 dan
toleransi kristal logger), tidak ada tepi hilang.

Pakai (cabut-colok daya host dulu, tunggu ~3 s, lalu):
  python tools/dev/tahap_stop/s0a_lse.py --logger COM18 60 [--catatan "pc1"]
  python tools/dev/tahap_stop/s0a_lse.py --analyse data/dev/s0a/<berkas>.csv
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[2]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import csv
import statistics
import sys
from datetime import datetime
from pathlib import Path

from run_bench import Logger  # noqa: E402

NOM_US = 250_000.0          # periode nominal PB1 (us): RTC 8 Hz, dibalik tiap tik
OUT = ROOT / "data" / "dev" / "s0a"


def analyse(text: str) -> dict:
    tick_hz, rises, falls, host = 72_000_000, [], [], []
    for ln in text.splitlines():
        ln = ln.strip()
        if ln.startswith("#RUN,BEGIN,"):
            tick_hz = int(ln.split(",")[5])         # #RUN,BEGIN,no,cmd,arg,tik_hz,build
        elif ln.startswith("e,"):
            _, t, r = ln.split(",")
            (rises if r == "1" else falls).append(int(t))
        elif ln.startswith("h,") and ",S0A," in ln:
            f = ln.split(",")
            host.append(f[3:])                      # tik, lse_ok, start_ms, rtc_cnt, bdcr
    res = {"tepi_naik": len(rises), "tepi_turun": len(falls)}
    if host:
        res["lse_ok"], res["lse_start_ms"], res["bdcr"] = host[-1][1], host[-1][2], host[-1][4]
    else:
        res["lse_ok"] = res["lse_start_ms"] = res["bdcr"] = "?"
    if len(rises) < 3:
        res["catatan_analisis"] = "tepi naik < 3: LSE mati atau PB1 tidak tersambung ke PA0"
        return res
    per = [(b - a) * 1e6 / tick_hz for a, b in zip(rises, rises[1:])]
    hilang = sum(1 for p in per if p > 1.5 * NOM_US)
    per_ok = [p for p in per if p <= 1.5 * NOM_US]
    span_us = (rises[-1] - rises[0]) * 1e6 / tick_hz
    n_per = round(span_us / NOM_US)                 # periode utuh di rentang (tahan tepi hilang)
    mean = span_us / n_per
    res.update({
        "periode_n": n_per,
        "periode_rata_us": round(mean, 3),
        "ppm_vs_logger": round((NOM_US / mean - 1) * 1e6, 1),   # + = LSE lebih cepat dari nominal
        "periode_min_us": round(min(per_ok), 3),
        "periode_maks_us": round(max(per_ok), 3),
        "periode_sd_us": round(statistics.stdev(per_ok), 3) if len(per_ok) > 1 else 0.0,
        "tepi_hilang": hilang,
        "catatan_analisis": "",
    })
    return res


def lulus(r: dict) -> bool:
    return (str(r.get("lse_ok")) == "1" and r.get("tepi_hilang", 1) == 0
            and abs(r.get("ppm_vs_logger", 1e9)) <= 50)


def cetak(r: dict) -> None:
    for k, v in r.items():
        print(f"  {k:18s} {v}")
    print("  HASIL             " + ("LULUS" if lulus(r) else "GAGAL / periksa"))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("detik", nargs="?", type=int, default=60)
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--catatan", default="")
    ap.add_argument("--analyse", metavar="CSV")
    a = ap.parse_args()

    if a.analyse:
        cetak(analyse(Path(a.analyse).read_text(errors="replace")))
        return

    OUT.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    lg = Logger(a.logger)
    try:
        raw = lg.command(7, a.detik, a.detik + 20)
    finally:
        lg.close()
    f = OUT / f"{stamp}_s0a.csv"
    f.write_bytes(raw)
    r = analyse(raw.decode(errors="replace"))
    r = {"waktu": stamp, "detik": a.detik, **r, "lulus": int(lulus(r)), "berkas": f.name, "catatan": a.catatan}
    ring = OUT / "ringkasan.csv"
    baru = not ring.exists()
    with ring.open("a", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(r.keys()), extrasaction="ignore")
        if baru:
            w.writeheader()
        w.writerow(r)
    print(f"log: {f}")
    cetak(r)
    rows = list(csv.DictReader(ring.open()))
    print(f"ringkasan.csv: {len(rows)} uji, lulus {sum(1 for x in rows if x.get('lulus') == '1')}")


if __name__ == "__main__":
    main()
