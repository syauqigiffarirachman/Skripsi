"""L6: pengendali run dari PC (15 Nov 2026).

Tiga papan: host (K1/K2, USB-C hanya daya), logger (USB CDC, perintah lewat nilai baud), attacker
(konsol CH340). Satu run:
  1. attacker dihentikan ('x');
  2. logger diperintah (baud = 1.000.000 + cmd x 1000 + detik) -> logger mulai merekam, mengirim
     S<detik> / W<detik>,<p> ke host lewat USART1;
  3. begitu baris ">START" dari host terlihat (papan siap), attacker dijalankan ('a' = 1/s, 'b' = 10/s);
  4. logger mengalirkan CSV (s = cuplikan INA226, e = tepi PB1, h = baris host) sampai "#CMD,SELESAI";
  5. attacker dihentikan, log mentah disimpan, lalu dianalisis.

Skenario:  A = normal 1 permintaan/s (attacker)      B = barrage 10 permintaan/s (attacker)
           IDLE = jendela tanpa permintaan (attacker diam) -> P_idle
           WA / WB = permintaan SINTETIS TIM4 host 1 Hz / 10 Hz (tanpa attacker, untuk uji)

Pakai:
  python tools/run_bench.py ping
  python tools/run_bench.py run A 60                     satu run
  python tools/run_bench.py plan A:30,B:30 --secs 60     sesi: IDLE di awal & akhir, urutan diacak
  python tools/run_bench.py plan K1.A:3,K2.A:3,K1.B:3,K2.B:3 --secs 60
                                                         sesi campuran K1/K2: urutan diacak penuh, host
                                                         di-flash ulang lewat ST-Link setiap arsitektur
                                                         berganti (butuh ST-Link host terpasang)
  python tools/run_bench.py plan ... --kering            cetak urutan saja
  python tools/run_bench.py nodebug                      reset host + DBGMCU_CR = 0
  python tools/run_bench.py analyse data/runs/<sesi>/<berkas>.csv

Setelah flash, OpenOCD menyalakan DBGMCU_CR = 0x307 (DBG_SLEEP: jam inti tetap jalan saat WFI) dan bit itu
baru hilang saat power-on reset. Uji 15 Nov: arus tidur K1 naik 0,2-0,7 mA bila bit menyala. Karena itu
flash_host() menolkan DBGMCU_CR setelah program; aksi 'nodebug' melakukan hal yang sama tanpa flash.

Kalibrasi arus: --rshunt (Ohm, bawaan 0.100 = nominal; hasil A2c ~0.080-0.083 BELUM final) dan
--offset (LSB shunt tanpa beban, bawaan -2). CSV mentah menyimpan nilai register, jadi analisis bisa
diulang dengan kalibrasi lain tanpa mengambil data lagi.
"""
import argparse
import csv
import hashlib
import random
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

TICK_HZ = 72_000_000
SHUNT_LSB_V = 2.5e-6
BUS_LSB_V = 1.25e-3
SCEN = {"A": (4, "a"), "B": (4, "b"), "IDLE": (4, None), "WA": (5, None), "WB": (6, None)}
END_COLS = ["arch", "secs", "p_ms", "uart", "t_obs_us", "t_sleep_us", "act_us", "resid_us", "arrived",
            "served", "gagal", "wfi", "spurious", "pin_salah", "rx_ok", "rx_bad", "rx_lost", "lat_n",
            "lat_min", "lat_mean", "lat_max", "lat_drop", "clk_hz"]
SUM_COLS = ["no", "t_stop", "n", "late", "dt_max", "i2c_err", "rec_ok", "rec", "sh_sum", "bus_sum",
            "e_lines", "er_drop", "seq_err", "h_lines", "h_ovf"]


ROOT = Path(__file__).resolve().parent.parent
ELF = {"K1": ROOT / ".pio/build/k1_sw/firmware.elf", "K2": ROOT / ".pio/build/k2_hw/firmware.elf"}
OCD = Path.home() / ".platformio/packages/tool-openocd"


# --------------------------------------------------------------------------- perangkat
def openocd(stlink, *cmds, timeout=60):
    """Jalankan OpenOCD sekali (ST-Link dipilih lewat nomor seri). Mengembalikan (ok, keluaran)."""
    a = [str(OCD / "bin/openocd"), "-s", str(OCD / "openocd/scripts"), "-c", f"adapter serial {{{stlink}}}",
         "-f", "interface/stlink.cfg", "-c", "transport select hla_swd", "-f", "target/stm32f1x.cfg"]
    for c in cmds:
        a += ["-c", c]
    p = subprocess.run(a, capture_output=True, text=True, timeout=timeout)
    return p.returncode == 0, p.stdout + p.stderr


def nodebug(stlink):
    """Reset host lalu nolkan DBGMCU_CR (DBG_SLEEP mati) agar arus tidur tidak terpengaruh debugger."""
    ok, out = openocd(stlink, "init", "reset run", "sleep 1500", "mww 0xE0042004 0", "mdw 0xE0042004", "shutdown")
    return ok and "0xe0042004: 00000000" in out.lower(), out


def flash_host(stlink, arch):
    elf = ELF[arch]
    if not elf.exists():
        raise FileNotFoundError(f"{elf} belum dibangun (pio run -e {elf.parent.name})")
    ok, out = openocd(stlink, f"program {{{elf.as_posix()}}} verify reset exit", timeout=90)
    if not ok or "Verified OK" not in out:
        raise RuntimeError(f"flash {arch} gagal:\n{out[-600:]}")
    ok, out = nodebug(stlink)
    if not ok:
        raise RuntimeError(f"DBGMCU_CR gagal dinolkan:\n{out[-600:]}")


def host_id(lg):
    raw = lg.command(3, 0, 5).decode(errors="replace")
    return next((l.split(",", 2)[2] for l in raw.splitlines() if l.startswith("h,") and ">ID" in l), None)


def wait_host(lg, arch=None, maks=25):
    """Tunggu host menjawab '?' (arsitektur cocok dan papan siap bila arch diberikan). Baris >ID atau None."""
    t = time.time()
    while time.time() - t < maks:
        ident = host_id(lg)
        if ident and (arch is None or (ident.split(",")[1] == arch and ident.split(",")[4] == "1")):
            return ident
        time.sleep(1)
    return None


def sha_short(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()[:12]


class Logger:
    def __init__(self, port):
        self.s = serial.Serial(port, 115200, timeout=0.05, write_timeout=1)
        self.s.dtr = True
        time.sleep(0.3)
        self.s.reset_input_buffer()

    def command(self, cmd, arg, timeout, on_line=None):
        """Kirim perintah lewat nilai baud, baca sampai '#CMD,SELESAI'. Mengembalikan bytes mentah."""
        self.s.reset_input_buffer()
        self.s.baudrate = 1_000_000 + cmd * 1000 + arg
        buf = bytearray()
        scanned = 0
        t_end = time.time() + timeout
        done = False
        while time.time() < t_end and not done:
            buf += self.s.read(65536)
            while True:                                   # baris utuh yang baru masuk
                i = buf.find(b"\n", scanned)
                if i < 0:
                    break
                line = bytes(buf[scanned:i]).strip()
                scanned = i + 1
                if on_line:
                    on_line(line)
                if line == b"#CMD,SELESAI":
                    done = True
        self.s.baudrate = 115200
        if not done:
            raise TimeoutError(f"logger tidak menyelesaikan perintah {cmd},{arg} dalam {timeout} s")
        return bytes(buf[:scanned])

    def close(self):
        self.s.close()


class Attacker:
    def __init__(self, port):
        self.s = serial.Serial(port, 115200, timeout=0.3, write_timeout=1)
        time.sleep(0.2)
        self.s.reset_input_buffer()

    def send(self, ch):
        self.s.write(ch.encode())
        self.s.flush()

    def status(self):
        self.s.reset_input_buffer()
        self.send("s")
        time.sleep(0.25)
        return self.s.read(300).decode(errors="replace").strip()

    def stop(self):
        self.send("x")
        st = self.status()
        if "berjalan=tidak" not in st:
            raise RuntimeError(f"attacker tidak berhenti: {st!r}")

    def close(self):
        self.s.close()


# --------------------------------------------------------------------------- analisis
def parse(text):
    r = {"s": [], "e": [], "h": [], "end": None, "sum": None, "lat": [], "begin": None, "status": None,
         "t_start": None, "t_endline": None}
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("s,"):
            f = line.split(",")
            if len(f) == 4:
                r["s"].append((int(f[1]), int(f[2]), int(f[3])))
        elif line.startswith("e,"):
            f = line.split(",")
            if len(f) == 3:
                r["e"].append((int(f[1]), int(f[2])))
        elif line.startswith("h,"):
            _, t, rest = line.split(",", 2)
            r["h"].append((int(t), rest))
            if rest.startswith(">START"):
                r["t_start"] = int(t)
            elif rest.startswith(">END,"):
                r["t_endline"] = int(t)
                v = rest.split(",")[1:]
                if len(v) == len(END_COLS):
                    r["end"] = {k: (x if k == "arch" else int(x)) for k, x in zip(END_COLS, v)}
            elif rest.startswith(">L,"):
                r["lat"] += [int(x) for x in rest.split(",")[1:]]
        elif line.startswith("#SUM,"):
            v = line.split(",")[1:]
            if len(v) == len(SUM_COLS):
                r["sum"] = {k: int(x) for k, x in zip(SUM_COLS, v)}
        elif line.startswith("#RUN,BEGIN,"):
            r["begin"] = line.split(",")[2:]
        elif line.startswith("#RUN,END,"):
            r["status"] = line.split(",")[3]
    return r


def analyse(text, rshunt=0.100, offset=-2.0):
    """Mengembalikan (hasil: dict, masalah: list[str])."""
    r = parse(text)
    out, bad = {}, []
    out["status"] = r["status"]
    if r["status"] != "OK":
        bad.append(f"status logger = {r['status']}")
    sm, en = r["sum"], r["end"]
    if sm is None:
        return out, bad + ["baris #SUM tidak ada"]
    # --- keutuhan aliran USB ---
    if len(r["s"]) != sm["n"]:
        bad.append(f"cuplikan diterima {len(r['s'])} != dikirim {sm['n']}")
    if sum(x[1] for x in r["s"]) != sm["sh_sum"] or sum(x[2] for x in r["s"]) != sm["bus_sum"]:
        bad.append("checksum shunt/bus tidak cocok")
    if len(r["e"]) != sm["e_lines"]:
        bad.append(f"tepi diterima {len(r['e'])} != dikirim {sm['e_lines']}")
    for k in ("late", "i2c_err", "rec", "er_drop", "seq_err", "h_ovf"):
        if sm[k]:
            bad.append(f"logger {k}={sm[k]}")
    out.update(n_samp=sm["n"], late=sm["late"], dt_max_us=sm["dt_max"] / TICK_HZ * 1e6, i2c_err=sm["i2c_err"])
    if en is None or r["t_start"] is None:
        return out, bad + ["hasil host (>END) tidak ada"]
    if len(r["lat"]) != en["lat_n"]:
        bad.append(f"latensi diterima {len(r['lat'])} != {en['lat_n']}")

    # --- jendela menurut logger: tepi naik pertama setelah >START .. tepi turun terakhir sebelum >END ---
    # Baris >START dicatat logger beberapa ratus us SETELAH byte terakhirnya, sedangkan t0 host terjadi
    # tepat setelah byte itu; beri kelonggaran 2 ms ke belakang.
    slack = TICK_HZ // 500
    ed = [(t, lv) for t, lv in r["e"] if r["t_start"] - slack <= t <= r["t_endline"]]
    rises = [t for t, lv in ed if lv == 1]
    falls = [t for t, lv in ed if lv == 0]
    if not rises or not falls:
        return out, bad + ["tepi PB1 tidak ada di jendela (kabel PB1 -> PA0?)"]
    t0, t1 = rises[0], falls[-1]
    T = (t1 - t0) / TICK_HZ
    low = 0
    prev_fall = None
    for t, lv in ed:
        if t < t0 or t > t1:
            continue
        if lv == 0:
            prev_fall = t
        elif prev_fall is not None:
            low += t - prev_fall
            prev_fall = None
    scpu_log = low / (t1 - t0) * 100
    host_T = en["t_obs_us"] / 1e6
    scpu_host = en["t_sleep_us"] / en["t_obs_us"] * 100 if en["t_obs_us"] else 0.0
    clk_ppm = (host_T / T - 1) * 1e6                      # jam host terhadap kristal logger
    out.update(arch=en["arch"], secs=en["secs"], T_obs_log_s=T, T_obs_host_s=host_T, clk_ppm=clk_ppm,
               S_cpu_log=scpu_log, S_cpu_host=scpu_host, arrived=en["arrived"], served=en["served"],
               gagal=en["gagal"], missed=en["arrived"] - en["served"] - en["gagal"], wfi=en["wfi"],
               spurious=en["spurious"], rx_bad=en["rx_bad"], rx_lost=en["rx_lost"], resid_us=en["resid_us"],
               pin_salah=en["pin_salah"])
    if abs(scpu_log - scpu_host) > 0.5:
        bad.append(f"S_cpu logger {scpu_log:.2f} vs host {scpu_host:.2f} berbeda > 0,5 poin")
    if abs(en["resid_us"]) > 2000:
        bad.append(f"sisa konsistensi host {en['resid_us']} us")
    if en["rx_bad"] or en["rx_lost"] or en["pin_salah"] or en["gagal"] or en["lat_drop"]:
        bad.append(f"host: bad={en['rx_bad']} hilang={en['rx_lost']} pin_salah={en['pin_salah']} "
                   f"gagal={en['gagal']} lat_drop={en['lat_drop']}")

    # --- latensi sign (siklus CYCCNT host). ms nominal = / clk nominal; ms nyata dikoreksi jam host ---
    if r["lat"]:
        k = 1e3 / en["clk_hz"]
        corr = T / host_T                                  # detik nyata per detik host
        m = sum(r["lat"]) / len(r["lat"])
        out.update(lat_n=len(r["lat"]), lat_mean_ms=m * k, lat_min_ms=min(r["lat"]) * k,
                   lat_max_ms=max(r["lat"]) * k, lat_mean_ms_nyata=m * k * corr)

    # --- energi: integrasi trapesium P = V x I pada [t0, t1], interpolasi linear di kedua batas ---
    def conv(x):
        i = (x[1] - offset) * SHUNT_LSB_V / rshunt
        return x[0], i, x[2] * BUS_LSB_V

    pts = [conv(x) for x in r["s"]]
    def at(t):                                             # interpolasi (i, v) pada waktu t
        for a, b in zip(pts, pts[1:]):
            if a[0] <= t <= b[0]:
                w = (t - a[0]) / (b[0] - a[0]) if b[0] != a[0] else 0
                return t, a[1] + w * (b[1] - a[1]), a[2] + w * (b[2] - a[2])
        return None
    p0, p1 = at(t0), at(t1)
    if p0 is None or p1 is None:
        return out, bad + ["cuplikan tidak mencakup jendela"]
    win = [p0] + [p for p in pts if t0 < p[0] < t1] + [p1]
    e = q = 0.0
    for a, b in zip(win, win[1:]):
        dt = (b[0] - a[0]) / TICK_HZ
        e += (a[1] * a[2] + b[1] * b[2]) / 2 * dt
        q += (a[1] + b[1]) / 2 * dt
    v_avg = sum(p[2] for p in win) / len(win)
    out.update(E_mJ=e * 1e3, P_avg_mW=e / T * 1e3, I_avg_mA=q / T * 1e3, V_avg=v_avg, n_win=len(win) - 2)
    if out["served"]:
        out["E_per_sign_mJ"] = e * 1e3 / out["served"]     # BELUM dikurangi P_idle (dikerjakan di sesi)
    # --- arus saat aktif vs tidur (keadaan PB1 pada saat cuplikan), untuk model I = Ia.D + Is.(1-D) ---
    ia, isl, j, state = [], [], 0, 0
    for p in win[1:-1]:
        while j < len(ed) and ed[j][0] <= p[0]:
            state = ed[j][1]
            j += 1
        (ia if state else isl).append(p[1])
    if ia:
        out["I_aktif_mA"] = sum(ia) / len(ia) * 1e3
    if isl:
        out["I_tidur_mA"] = sum(isl) / len(isl) * 1e3
    if ia and isl:
        d = 1 - scpu_log / 100
        out["I_model_mA"] = out["I_aktif_mA"] * d + out["I_tidur_mA"] * (1 - d)
    return out, bad


def fmt(res, bad):
    keys = ["arch", "secs", "T_obs_log_s", "T_obs_host_s", "clk_ppm", "S_cpu_log", "S_cpu_host", "arrived",
            "served", "missed", "wfi", "spurious", "lat_n", "lat_mean_ms", "lat_min_ms", "lat_max_ms",
            "lat_mean_ms_nyata", "n_samp", "late", "dt_max_us", "E_mJ", "P_avg_mW", "I_avg_mA", "V_avg",
            "I_aktif_mA", "I_tidur_mA", "I_model_mA", "E_per_sign_mJ"]
    parts = []
    for k in keys:
        if k in res:
            v = res[k]
            parts.append(f"{k}={v:.4f}" if isinstance(v, float) else f"{k}={v}")
    return "  ".join(parts) + ("\n  MASALAH: " + "; ".join(bad) if bad else "\n  pemeriksaan: semua lolos")


# --------------------------------------------------------------------------- run
def do_run(lg, at, scen, secs, out_path, args):
    cmd, mode = SCEN[scen]
    at.stop()
    started = []

    def on_line(line):
        if not started and line.startswith(b"h,") and b">START" in line:
            started.append(time.time())
            if mode:
                at.send(mode)                             # papan siap -> attacker mulai

    raw = lg.command(cmd, secs, secs + 20, on_line)
    at_status = at.status() if mode else ""
    at.stop()
    out_path.write_bytes(raw)
    res, bad = analyse(raw.decode(errors="replace"), args.rshunt, args.offset)
    if mode and "berjalan=ya" not in at_status:
        bad.append(f"attacker tidak berjalan di akhir run: {at_status!r}")
    res["scen"], res["file"] = scen, out_path.name
    return res, bad


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("aksi", choices=["ping", "run", "plan", "analyse", "nodebug"])
    ap.add_argument("arg", nargs="*")
    ap.add_argument("--logger", default="COM18")
    ap.add_argument("--attacker", default="COM16")
    ap.add_argument("--secs", type=int, default=60)
    ap.add_argument("--rshunt", type=float, default=0.100)
    ap.add_argument("--offset", type=float, default=-2.0)
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("--jeda", type=float, default=3.0, help="jeda antar run (s)")
    ap.add_argument("--tanpa-idle", action="store_true")
    ap.add_argument("--catatan", default="")
    ap.add_argument("--stlink", default="$", help="nomor seri ST-Link host (untuk flash/reset)")
    ap.add_argument("--kering", action="store_true", help="plan: cetak urutan saja, tanpa menjalankan")
    args = ap.parse_args()

    if args.aksi == "analyse":
        for f in args.arg:
            res, bad = analyse(Path(f).read_text(errors="replace"), args.rshunt, args.offset)
            print(f"{f}\n  {fmt(res, bad)}")
        return

    if args.aksi == "nodebug":
        ok, out = nodebug(args.stlink)
        print("DBGMCU_CR = 0" if ok else "GAGAL:\n" + out[-600:])
        return

    # --- susun rencana dulu (tanpa perangkat), agar --kering bisa dipakai tanpa papan ---
    seed = args.seed if args.seed is not None else int(time.time())
    plan, campur, secs = [], False, args.secs             # plan: daftar (arch | None, skenario)
    if args.aksi == "run":
        plan = [(None, args.arg[0].upper())]
        secs = int(args.arg[1]) if len(args.arg) > 1 else args.secs
    elif args.aksi == "plan":
        for item in args.arg[0].split(","):
            sc, n = item.split(":")
            ar, _, sc = sc.upper().rpartition(".")
            if ar and ar not in ELF:
                raise SystemExit(f"arsitektur tidak dikenal: {ar}")
            campur |= bool(ar)
            plan += [(ar or None, sc)] * int(n)
        if campur and any(a is None for a, _ in plan):
            raise SystemExit("plan campuran: setiap butir harus berawalan K1. atau K2.")
        if any(sc not in SCEN for _, sc in plan):
            raise SystemExit("skenario tidak dikenal (pilihan: " + ", ".join(SCEN) + ")")
        rng = random.Random(seed)
        rng.shuffle(plan)
        if not args.tanpa_idle:                           # IDLE tiap arsitektur di awal dan akhir sesi
            archs = sorted({a for a, _ in plan}) if campur else [None]
            awal, akhir = archs[:], archs[:]
            rng.shuffle(awal)
            rng.shuffle(akhir)
            plan = [(a, "IDLE") for a in awal] + plan + [(a, "IDLE") for a in akhir]
        if args.kering:
            ganti = sum(1 for x, y in zip(plan, plan[1:]) if x[0] != y[0])
            print(f"seed={seed}  {len(plan)} run, ganti arsitektur (flash) {ganti} kali")
            for i, (a, sc) in enumerate(plan, 1):
                print(f"  {i:3d} {a or '-'} {sc}")
            return

    lg = Logger(args.logger)
    try:
        ident = host_id(lg)
        print("host:", ident or "TIDAK MENJAWAB")
        if args.aksi == "ping" or (ident is None and not campur):
            return
        arch = ident.split(",")[1] if ident else "?"
        if not campur and ident.split(",")[4] != "1":
            print("host belum siap (grup P-256 / DS28E38 tidak ditemukan)")
            return
        at = Attacker(args.attacker)
        print("attacker:", at.status())
        tag = "K1K2" if campur else arch
        sesi = ROOT / "data" / "runs" / f"{datetime.now():%Y%m%d_%H%M%S}_{tag}"
        sesi.mkdir(parents=True, exist_ok=True)
        elf_info = "".join(f"elf_{a}={ELF[a].relative_to(ROOT).as_posix()} sha256={sha_short(ELF[a])}\n"
                           for a in ELF if campur and ELF[a].exists())
        rencana = ",".join((a + "." if a else "") + sc for a, sc in plan)
        (sesi / "sesi.txt").write_text(
            f"host={ident}\nseed={seed}\nsecs={secs}\nplan={rencana}\nrshunt={args.rshunt}\n"
            f"offset={args.offset}\n{elf_info}catatan={args.catatan}\n", encoding="utf-8")
        rows = []
        for i, (want, scen) in enumerate(plan, 1):
            res, bad = {"scen": scen}, []
            if want and want != arch:                     # ganti firmware host lewat ST-Link
                t = time.time()
                try:
                    flash_host(args.stlink, want)
                    ident = wait_host(lg, want)
                    if ident is None:
                        raise RuntimeError("host tidak menjawab / belum siap setelah flash")
                    arch = want
                    res["flash_s"] = round(time.time() - t, 1)
                    print(f"  flash {want} + DBGMCU_CR=0 dalam {res['flash_s']} s: {ident}", flush=True)
                except Exception as ex:
                    print(f"  BERHENTI: {ex}")
                    break
            f = sesi / f"{i:03d}_{arch}_{scen}.csv"
            print(f"[{i}/{len(plan)}] {arch} {scen} {secs} s ...", flush=True)
            try:
                r2, bad = do_run(lg, at, scen, secs, f, args)
                res.update(r2)
            except Exception as ex:                       # run gagal: catat, lanjut ke berikutnya
                res["file"], bad = f.name, [f"pengecualian: {ex}"]
                try:
                    at.send("x")
                except Exception:
                    pass
            if want and res.get("arch") not in (None, want):
                bad.append(f"arsitektur host {res.get('arch')} != rencana {want}")
            res["urut"], res["masalah"] = i, "; ".join(bad)
            rows.append(res)
            print("  " + fmt(res, bad), flush=True)
            cols = ["urut"] + sorted({k for r in rows for k in r} - {"urut"})
            with (sesi / "ringkasan.csv").open("w", newline="", encoding="utf-8") as fh:
                w = csv.DictWriter(fh, fieldnames=cols)
                w.writeheader()
                w.writerows(rows)
            if bad and wait_host(lg, maks=5) is None:     # host diam setelah run gagal: jangan buang run
                print("  host tidak menjawab; sesi dihentikan (cek host, atau 'nodebug' untuk reset)")
                break
            time.sleep(args.jeda)
        at.close()
        print(f"sesi: {sesi}   run bermasalah: {sum(1 for r in rows if r['masalah'])}/{len(rows)}")
    finally:
        lg.close()


if __name__ == "__main__":
    sys.exit(main())
