"""Uji jalur galat varian Stop (K1S/K2S), 18 Nov 2026. Tanpa OpenOCD dan tanpa mengubah kabel.

Gangguan diberikan lewat attacker SAAT jendela berjalan (host tidur Stop di antara permintaan):
  G1  attacker berhenti total di detik 25   -> host harus tetap tidur sampai jendela habis, jendela 60 s utuh,
                                               S_cpu logger = host, tidak ada crash
  G2  attacker dimatikan lalu dinyalakan ulang (detik 20 dan 21) -> urutan nomor frame melompat/restart; host harus
                                               mencatat restart (bukan hilang/rusak) dan tetap lancar
  G3  attacker bolak-balik cepat (hidup/mati tiap 1,5 s, detik 15-45) -> garis PA3 aktif/diam berganti terus; tak
                                               boleh ada frame rusak, bangun palsu berlebihan, atau crash
Setelah tiap gangguan satu run normal A 60 s harus lolos penuh (tanda host sehat, tidak terkunci).

Pakai: python tools/uji_galat_stop.py [--logger COM18 --attacker COM24 --skenario B]
Keluaran: data/dev/err/stop_<waktu>_G*.csv dan ringkasan di layar.
"""
import argparse
import sys
import threading
import time
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_bench as rb  # noqa: E402
import kalibrasi_param as kp  # noqa: E402

OUT = Path(__file__).resolve().parent.parent / "data" / "dev" / "err"


def jalankan(lg, at, scen, secs, rencana, nama, args):
    """Satu run dengan jadwal gangguan: rencana = [(detik_sejak_START, aksi)], aksi = karakter attacker."""
    cmd, mode = rb.SCEN[scen]
    at.stop()
    t_start = []

    def peristiwa():
        t0 = t_start[0]
        for dt, aksi in rencana:
            sisa = t0 + dt - time.time()
            if sisa > 0:
                time.sleep(sisa)
            at.send(aksi if aksi != "mode" else mode)

    def on_line(line):
        if not t_start and line.startswith(b"h,") and b">START" in line:
            t_start.append(time.time())
            at.send(mode)
            threading.Thread(target=peristiwa, daemon=True).start()

    raw = lg.command(cmd, secs, secs + 25, on_line)
    st = at.status()
    at.stop()
    f = OUT / f"stop_{datetime.now():%Y%m%d_%H%M%S}_{nama}.csv"
    f.write_bytes(raw)
    res, bad = rb.analyse(raw.decode(errors="replace"), args.rshunt, args.offset)
    r = rb.parse(raw.decode(errors="replace"))
    return res, bad, f, st, r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--attacker", default="COM24")
    ap.add_argument("--skenario", default="B", choices=["A", "B"])
    ap.add_argument("--rshunt", type=float, default=kp.RSHUNT_EFEKTIF)
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB)
    args = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    lg = rb.Logger(args.logger)
    at = rb.Attacker(args.attacker)
    ident = rb.host_id(lg)
    print("host:", ident or "TIDAK MENJAWAB")
    if not ident:
        return 2
    mode = rb.SCEN[args.skenario][1]
    gang = {
        "G1_berhenti_25s": [(25, "x")],
        "G2_restart_20s": [(20, "x"), (21, "mode")],
        "G3_bolak_balik": sum([[(15 + 3 * i, "x"), (16.5 + 3 * i, "mode")] for i in range(10)], []),
    }
    ringkas, ok_semua = [], True
    for nama, rencana in gang.items():
        try:
            res, bad, f, st, r = jalankan(lg, at, args.skenario, 60, rencana, nama, args)
        except Exception as ex:  # noqa: BLE001
            print(f"[{nama}] PENGECUALIAN: {ex}")
            ok_semua = False
            continue
        # kriteria khusus uji galat: jendela utuh, S_cpu cocok, tak ada rusak, host hidup
        masalah = []
        if abs(res["T_obs_host_s"] - 60) > 0.6:
            masalah.append(f"jendela {res['T_obs_host_s']:.2f} s (harus ~60)")
        if abs(res["S_cpu_log"] - res["S_cpu_host"]) > 0.5:
            masalah.append("S_cpu logger vs host > 0,5")
        if res["rx_bad"]:
            masalah.append(f"frame rusak {res['rx_bad']}")
        if res["served"] == 0:
            masalah.append("tidak ada permintaan terlayani")
        if res["spurious"] > 20:
            masalah.append(f"bangun palsu {res['spurious']}")
        hasil = "LULUS" if not masalah else "GAGAL: " + "; ".join(masalah)
        ok_semua &= not masalah
        print(f"[{nama}] {hasil} | arrived={res['arrived']} served={res['served']} rusak={res['rx_bad']} "
              f"hilang={res['rx_lost']} spurious={res['spurious']} S_cpu_log={res['S_cpu_log']:.2f} "
              f"S_cpu_host={res['S_cpu_host']:.2f} T={res['T_obs_host_s']:.2f} s ({f.name})", flush=True)
        ringkas.append((nama, hasil))
        # sehat-kah host sesudahnya: satu run normal A
        time.sleep(3)
        res2, bad2, f2, st2, _ = jalankan(lg, at, "A", 60, [], f"{nama}_pulih_A", args)
        sehat = not bad2
        ok_semua &= sehat
        print(f"    run pulih A: {'OK' if sehat else 'GAGAL ' + '; '.join(bad2)} | served={res2['served']}/{res2['arrived']}",
              flush=True)
    print("\nRINGKASAN:", "SEMUA LULUS" if ok_semua else "ADA YANG GAGAL")
    lg.close()
    at.close()
    return 0 if ok_semua else 1


if __name__ == "__main__":
    sys.exit(main())
