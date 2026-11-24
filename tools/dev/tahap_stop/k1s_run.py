"""Run K1 + Stop mode (arsitektur K1S) dengan host MODE OTOMATIS (17 Nov 2026).

Dipakai selama jalur logger -> host (PA9 logger -> PA10 host) tidak tersambung: firmware host_k1_stop_auto
memulai sendiri jendela S<K1S_AUTO_S> berulang (jeda 8 s + jendela 60 s + ukur HCLK 2 s + kirim hasil) dan
melapor lewat jalur host -> logger yang hidup. Skrip ini:
  1. merekam lewat logger (perintah 7 = rekam saja) selama ~2 siklus host;
  2. begitu '>START' terlihat menyalakan attacker (A = 1 Hz, B = 10 Hz, IDLE = tidak), begitu '>END' mematikan;
  3. memotong SATU run lengkap (>START .. >DONE pertama), menambahkan baris #SUM sintetis dari rekaman
     (jumlah cuplikan, checksum, celah terbesar) dan memanggil run_bench.analyse (pemeriksaan sama persis
     dengan run biasa: S_cpu logger vs host <= 0,5 poin, bad/hilang/gagal = 0, latensi dll.).
Keluaran: berkas CSV di data/dev/k1s/ dan ringkasan metrik.

Pakai: python tools/dev/tahap_stop/k1s_run.py A|B|IDLE [--secs 60] [--logger COM18 --attacker COM22] [--catatan ...]
       python tools/dev/tahap_stop/k1s_run.py --analyse data/dev/k1s/<berkas>.csv
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[2]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import sys
import time
from datetime import datetime
from pathlib import Path

import run_bench as rb  # noqa: E402
import kalibrasi_param as kp  # noqa: E402

OUT = ROOT / "data" / "dev" / "k1s"
GAP_S, CAL_S, KIRIM_S = 8, 2, 4          # jeda host, ukur HCLK, kirim hasil (perkiraan)


def potong_satu_run(text):
    """Pertahankan semua baris s/e, tetapi hanya baris h dan penanda satu run lengkap. Mengembalikan teks."""
    lines = text.splitlines()
    h_idx = [i for i, l in enumerate(lines) if l.startswith("h,")]
    start = next((i for i in h_idx if ">START" in lines[i] and any(">DONE" in lines[j] for j in h_idx if j > i)), None)
    if start is None:
        return None
    done = next(j for j in h_idx if j > start and ">DONE" in lines[j])
    keep_h = set(i for i in h_idx if start <= i <= done)
    return "\n".join(l for i, l in enumerate(lines) if not l.startswith("h,") or i in keep_h)


def tambah_sum(text):
    """Baris #SUM sintetis (logger perintah 7 tidak menulisnya). late = celah cuplik > 3 ms."""
    n, sh, bus, ts, ne = 0, 0, 0, [], 0
    for l in text.splitlines():
        if l.startswith("s,"):
            f = l.split(",")
            if len(f) == 4:
                n += 1; ts.append(int(f[1])); sh += int(f[2]); bus += int(f[3])
        elif l.startswith("e,"):
            ne += 1
    gaps = [b - a for a, b in zip(ts, ts[1:])]
    late = sum(1 for g in gaps if g > 3.0e-3 * rb.TICK_HZ)
    dt_max = max(gaps) if gaps else 0
    sumline = "#SUM," + ",".join(str(x) for x in (0, 0, n, late, dt_max, 0, 0, 0, sh, bus, ne, 0, 0, 0, 0))
    out, done = [], False
    for l in text.splitlines():
        if l.startswith("#RUN,END") and not done:
            out.append(sumline)
            done = True
        out.append(l)
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("scen", nargs="?", choices=["A", "B", "IDLE"], default="A")
    ap.add_argument("--secs", type=int, default=60)
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--attacker", default="COM22")
    ap.add_argument("--rshunt", type=float, default=kp.RSHUNT_EFEKTIF)
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB)
    ap.add_argument("--catatan", default="")
    ap.add_argument("--analyse", metavar="CSV")
    a = ap.parse_args()
    if a.analyse:
        res, bad = rb.analyse(Path(a.analyse).read_text(errors="replace"), a.rshunt, a.offset)
        print(rb.fmt(res, bad))
        return
    OUT.mkdir(parents=True, exist_ok=True)
    mode = {"A": "a", "B": "b", "IDLE": None}[a.scen]
    at = rb.Attacker(a.attacker)
    lg = rb.Logger(a.logger)

    def on_line(line):
        if line.startswith(b"h,") and b">START" in line and mode:
            at.send(mode)
        elif line.startswith(b"h,") and b">END" in line:
            at.send("x")

    try:
        at.stop()
        siklus = GAP_S + a.secs + CAL_S + KIRIM_S
        # Logger menghentikan rekaman perintah 7 begitu melihat '>DONE' pertama. Rekaman SINKRON (tanpa attacker)
        # berakhir tepat sesudah satu siklus host selesai, jadi rekaman UTAMA langsung dimulai di jeda 8 s host
        # dan memuat '>START' .. '>DONE' satu run utuh.
        lg.command(7, siklus + 25, siklus + 55)
        raw = lg.command(7, siklus + 20, siklus + 50, on_line)
    finally:
        try:
            at.stop()
        except Exception as e:  # noqa: BLE001
            print("peringatan: attacker tidak berhenti:", e)
        lg.close()
        at.close()
    text = raw.decode(errors="replace")
    mentah = OUT / f"{datetime.now():%Y%m%d_%H%M%S}_k1s_{a.scen}_{a.catatan or 'run'}_mentah.csv"
    mentah.write_bytes(raw)
    run = potong_satu_run(text)
    if run is None:
        print("tidak ada run lengkap (>START..>DONE) di rekaman:", mentah)
        return
    f = mentah.with_name(mentah.name.replace("_mentah", ""))
    f.write_text(tambah_sum(run))
    res, bad = rb.analyse(f.read_text(), a.rshunt, a.offset)
    print(f"log: {f}")
    print(rb.fmt(res, bad))


if __name__ == "__main__":
    main()
