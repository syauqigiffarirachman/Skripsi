"""Deep sleep langkah 7 / S2: host Stop dibangunkan FRAME attacker, terima utuh (17 Nov 2026).

Host (env host_s2_rx) tidur Stop; tiap frame attacker (36 B) membangunkan host, diterima utuh (sinkron bit
byte pertama), "kerja" 20 ms, tidur lagi. Tiap jendela 10 s host mengirim lewat USART1 (logger: baris h):
  S2,<jendela>,<t_obs_us>,<t_sleep_us>,<n_stop>,<palsu>,<ok>,<bad>,<hilang>,<restart>,<dilayani>,
     <seq_awal>,<seq_akhir>,<sync_min_us>,<sync_maks_us>,<frame_wake>,<alarm_wake>
Alur skrip: attacker 'x'; rekam logger (perintah 7); begitu laporan host pertama terlihat kirim 'a' (1 Hz) atau
'b' (10 Hz); setelah --jendela laporan berikutnya kirim 'x' dan baca status (seq = jumlah frame terkirim);
rekam beberapa jendela lagi. Kriteria lulus: jumlah ok seluruh jendela = frame terkirim attacker, bad = 0,
hilang = 0, palsu = 0, selisih T_sleep (RTC host dikoreksi --ppm vs PB1 logger) < 2 ms per jendela 10 s (0,02 %; kuantisasi RTC 30,5 us per bangun + sinkron RSF),
jumlah tidur logger = n_stop.

Pakai: python tools/s2_rx.py A|B [--logger COM18 --attacker COM22 --jendela 6 --catatan ...]
       python tools/s2_rx.py --analyse data/dev/s2/<berkas>.csv [--sent N]
"""
import argparse
import statistics
import sys
import time
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from run_bench import Logger, Attacker  # noqa: E402

OUT = Path(__file__).resolve().parent.parent / "data" / "dev" / "s2"
LSB_V = 2.5e-6
COLS = ["win", "t_obs", "t_sleep", "n_stop", "palsu", "ok", "bad", "lost", "restart", "served", "seq_awal",
        "seq_akhir", "sync_min", "sync_max", "fwake", "awake"]


def analyse(text, ppm=-47.3, rshunt=0.100, offset=-2.0, sent=None):
    hz, s, edges, rep = 72_000_000, [], [], []
    for ln in text.splitlines():
        ln = ln.strip()
        if ln.startswith("#RUN,BEGIN,"):
            hz = int(ln.split(",")[5])
        elif ln.startswith("s,"):
            _, t, sh, _bus = ln.split(",")
            s.append((int(t), int(sh)))
        elif ln.startswith("e,"):
            _, t, r = ln.split(",")
            edges.append((int(t), r == "1"))
        elif ln.startswith("h,"):
            f = ln.split(",")
            if len(f) >= 3 + len(COLS) and f[2] == "S2" and f[3].isdigit():
                rep.append((int(f[1]), dict(zip(COLS, map(int, f[3:3 + len(COLS)])))))
            elif len(f) > 3 and f[2] == "S2":
                print("  host:", ",".join(f[2:]))
    edges.sort()
    lows = [(a[0], b[0]) for a, b in zip(edges, edges[1:]) if not a[1] and b[1]]
    us = lambda dt: dt * 1e6 / hz  # noqa: E731

    def cur(iv, pre_ms, post_ms):
        return [x[1] for x in s if iv[0] + pre_ms * hz / 1000 <= x[0] <= iv[1] - post_ms * hz / 1000]

    def to_i(v):
        return (statistics.fmean(v) - offset) * LSB_V / rshunt * 1e3 if v else float("nan")

    hasil = []
    for (ta, ra), (tb, rb) in zip(rep, rep[1:]):
        if rb["win"] != ra["win"] + 1:
            continue
        lw = [(a, b) for a, b in lows if ta - 0.020 * hz < a and b < tb]
        ts_log = sum(us(b - a) for a, b in lw)
        host_c = rb["t_sleep"] / (1 + ppm * 1e-6)
        sl = []
        for iv in lw:
            sl += cur(iv, 3, 1)
        hasil.append({
            "jendela": rb["win"], "ok": rb["ok"], "bad": rb["bad"], "hilang": rb["lost"], "restart": rb["restart"],
            "palsu": rb["palsu"], "dilayani": rb["served"], "n_stop_host": rb["n_stop"], "n_tidur_logger": len(lw),
            "frame_wake": rb["fwake"], "selisih_Tsleep_us": round(host_c - ts_log, 1),
            "Scpu_host_%": round(rb["t_sleep"] / rb["t_obs"] * 100, 3),
            "Scpu_logger_%": round(ts_log / us(tb - ta) * 100, 3),
            "sync_us_min/maks": f"{rb['sync_min']}/{rb['sync_max']}", "I_tidur_mA": round(to_i(sl), 3),
        })
    return {"laporan_host": len(rep), "tepi": len(edges), "jendela_banding": hasil, "rep": rep, "sent": sent}


def lulus_jendela(j):
    return (j["bad"] == 0 and j["hilang"] == 0 and j["palsu"] == 0 and abs(j["selisih_Tsleep_us"]) < 2000
            and j["n_stop_host"] == j["n_tidur_logger"])


def cetak(r):
    print(f"  laporan host: {r['laporan_host']}  tepi PB1: {r['tepi']}")
    for j in r["jendela_banding"]:
        print("  " + "  ".join(f"{k}={v}" for k, v in j.items()) + ("  -> OK" if lulus_jendela(j) else "  -> PERIKSA"))
    # Jumlah ok seluruh jendela setelah laporan pertama = frame terkirim.
    tot = {k: sum(w[k] for _, w in r["rep"][1:]) for k in ("ok", "bad", "lost", "palsu", "restart")}
    print("  total (setelah laporan pertama):", tot)
    if r["sent"] is not None:
        sent_ok = tot["ok"] == r["sent"] and tot["bad"] == 0 and tot["lost"] == 0 and tot["palsu"] == 0
        print(f"  frame terkirim attacker = {r['sent']}  ->  {'LULUS' if sent_ok else 'GAGAL'}")
    ok_all = all(lulus_jendela(j) for j in r["jendela_banding"]) and r["jendela_banding"]
    print("  semua jendela lulus:", bool(ok_all))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", nargs="?", choices=["A", "B"], default="A")
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--attacker", default="COM22")
    ap.add_argument("--jendela", type=int, default=6, help="jumlah jendela laporan dengan attacker aktif")
    ap.add_argument("--win-s", type=int, default=10, help="lama jendela host (s)")
    ap.add_argument("--catatan", default="")
    ap.add_argument("--ppm", type=float, default=-47.3)
    ap.add_argument("--rshunt", type=float, default=0.100)
    ap.add_argument("--offset", type=float, default=-2.0)
    ap.add_argument("--analyse", metavar="CSV")
    ap.add_argument("--sent", type=int, default=None)
    a = ap.parse_args()
    if a.analyse:
        cetak(analyse(Path(a.analyse).read_text(errors="replace"), a.ppm, a.rshunt, a.offset, a.sent))
        return
    OUT.mkdir(parents=True, exist_ok=True)
    at = Attacker(a.attacker)
    lg = Logger(a.logger)
    state = {"n": 0, "sent": None, "status": ""}
    ch = "a" if a.mode == "A" else "b"

    def on_line(line):
        if line.startswith(b"h,") and b",S2," in line:
            f = line.split(b",")
            if len(f) > 4 and f[3].isdigit():
                state["n"] += 1
                if state["n"] == 1:
                    at.send(ch)
                elif state["n"] == 1 + a.jendela:
                    at.send("x")
                    time.sleep(0.3)
                    state["status"] = at.status()

    try:
        at.stop()
        secs = a.win_s * (a.jendela + 4) + 6
        raw = lg.command(7, secs, secs + 25, on_line)
    finally:
        try:
            at.stop()
        except Exception as e:  # noqa: BLE001
            print("peringatan: attacker tidak berhenti:", e)
        lg.close()
        at.close()
    st = state["status"]
    sent = None
    if "seq=" in st:
        sent = int(st.split("seq=")[1].split()[0])
    f = OUT / f"{datetime.now():%Y%m%d_%H%M%S}_s2_{a.mode}_{a.catatan or 'run'}.csv"
    f.write_bytes(raw)
    print(f"log: {f}\n  status attacker saat dihentikan: {st!r}")
    cetak(analyse(raw.decode(errors="replace"), a.ppm, a.rshunt, a.offset, sent))


if __name__ == "__main__":
    main()
