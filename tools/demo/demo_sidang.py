"""Demo sidang: empat run (K1S/K2S x A/B), jendela 60 s, dipandu dari PC (19 Nov 2026).

Rancangan: docs/superpowers/specs/2026-11-19-demo-sidang-design.md. Skrip tipis di atas tools/1_akuisisi/run_bench.py
(Logger, host_id, wait_host, flash_host, analyse dipakai ulang, tidak disalin) sehingga semua angka dihitung
dengan rumus yang sama dengan data skripsi.
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import argparse
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

import run_bench  # noqa: E402
import kalibrasi_param as kp  # noqa: E402

ROOT = run_bench.ROOT

URUTAN_SESI = [("K1S", "A"), ("K1S", "B"), ("K2S", "A"), ("K2S", "B")]   # satu kali flash di depan penguji

CATATAN_STLINK = ("Catatan: ST-Link host terpasang selama demo (perlu untuk flash K1<->K2), sehingga arus diam terbaca "
                  "sedikit lebih rendah (0,03-0,11 mA, catatan 15 Nov); angka dapat sedikit berbeda dari tabel skripsi.")


class LiveMeter:
    """Mengubah baris CSV logger menjadi tampilan langsung: arus rata-rata per detik logger + peristiwa host."""

    PENTING = (b">START", b">END", b">DONE", b">ERR")

    def __init__(self, rshunt=0.100, offset=-2.0, tick_hz=run_bench.TICK_HZ):
        self.rshunt, self.offset, self.tick_hz = rshunt, offset, tick_hz
        self._sec = None
        self._sum = 0.0
        self._n = 0
        self._edges = 0

    def _tutup(self):
        if self._n == 0:
            return []
        baris = f"t={self._sec:3d} s  I={self._sum / self._n:6.2f} mA  tepi PB1={self._edges}"
        self._sum, self._n = 0.0, 0
        return [baris]

    def feed(self, line):
        if line.startswith(b"s,"):
            f = line.split(b",")
            sec = int(f[1]) // self.tick_hz
            mA = (int(f[2]) - self.offset) * run_bench.SHUNT_LSB_V / self.rshunt * 1e3
            keluar = []
            if self._sec is not None and sec != self._sec:
                keluar = self._tutup()
            self._sec = sec
            self._sum += mA
            self._n += 1
            return keluar
        if line.startswith(b"e,"):
            self._edges += 1
            return []
        if line.startswith(b"h,") and any(k in line for k in self.PENTING):
            return ["host: " + line.split(b",", 2)[2].decode(errors="replace")]
        return []


def tabel_ringkas(rows):
    kolom = [("arch", "{}", 5), ("scen", "{}", 6), ("I_avg_mA", "{:.2f}", 10), ("arrived", "{}", 8),
             ("served", "{}", 7), ("N_rusak", "{}", 8), ("N_lwt", "{}", 6), ("lat_mean_ms_nyata", "{:.1f}", 18),
             ("TL_pct", "{:.1f}", 7)]
    baris = ["".join(f"{k:<{w}}" for k, _, w in kolom)]
    for r in rows:
        sel = []
        for k, f, w in kolom:
            v = r.get(k)
            sel.append(f"{(f.format(v) if v is not None else '-'):<{w}}")
        teks = "".join(sel)
        if r.get("masalah"):
            teks += f"{'PERIKSA' if r.get('file') else 'GAGAL'}: {r['masalah']}"   # selesai-tapi-bercatatan != gagal
        baris.append(teks)
    return "\n".join(baris)


def simpan_run(folder, urut, arch, scen, raw):
    p = Path(folder) / f"{urut:03d}_{arch}_{scen}.csv"
    p.write_bytes(raw)
    return p


# --------------------------------------------------------------------------- orkestrasi
class HostError(Exception):
    pass


class HostTidakMenjawab(HostError):
    pass


class HostSalahArsitektur(HostError):
    pass


def jalankan_run(lg, arch, scen, secs=60, hitung=5, jeda=5, *, out=print, sleep=time.sleep, on_start=None,
                 analisis=run_bench.analyse, rshunt=0.100, offset=-2.0):
    """Satu run: HOST SIAP -> hitung mundur -> BISA MULAI MENYERANG -> jeda -> jendela host. (raw, hasil, masalah)."""
    ident = run_bench.host_id(lg)
    if ident is None:
        raise HostTidakMenjawab("host tidak menjawab '?' (periksa kabel logger PA9 -> host PA10, daya host, firmware)")
    f = ident.split(",")
    if len(f) < 5 or f[1] != arch or f[4] != "1":
        raise HostSalahArsitektur(f"host berisi {ident!r}, diminta {arch} dan siap; ketik 'mode {arch[:2].lower()}'")
    out(f"=== HOST SIAP ({arch}) ===")
    for n in range(hitung, 0, -1):
        out(f"  mulai menyerang dalam {n} ...")
        sleep(1)
    huruf = "a" if scen == "A" else "b"
    laju = "1 permintaan/s" if scen == "A" else "10 permintaan/s (barrage)"
    out(f"=== BISA MULAI MENYERANG - ketik {huruf} di serial monitor attacker ({laju}) ===")
    sleep(jeda)

    meter = LiveMeter(rshunt, offset)
    mulai = []

    def on_line(line):
        for t in meter.feed(line):
            out(t)
        if not mulai and line.startswith(b"h,") and b">START" in line:
            mulai.append(1)
            if on_start:
                on_start()

    try:
        raw = lg.command(4, secs, secs + 20, on_line)
    finally:
        try:
            lg.s.baudrate = 115200                         # jangan biarkan port macet di baud perintah
        except Exception:
            pass
    res, bad = analisis(raw.decode(errors="replace"), rshunt, offset)
    return raw, res, bad


def ganti_mode(lg, arch, stlink="$", *, out=print, flash=run_bench.flash_host, wait=run_bench.wait_host):
    out(f"Mengganti firmware host ke {arch} (ST-Link {stlink}) ...")
    flash(stlink, arch)
    ident = wait(lg, arch, 40)
    if ident is None:
        raise HostTidakMenjawab(f"host tidak siap sebagai {arch} setelah flash")
    out(f"Host siap: {ident}")
    return ident


def jalankan_sesi(lg, folder, *, secs=60, hitung=5, jeda=5, stlink="$", tunggu=input, out=print, sleep=time.sleep,
                  flash=run_bench.flash_host, wait=run_bench.wait_host, analisis=run_bench.analyse,
                  on_start_factory=None, rshunt=0.100, offset=-2.0):
    """Empat run URUTAN_SESI; satu run gagal tidak menghentikan sesi. Satu dict per run."""
    ident = run_bench.host_id(lg)
    parts = ident.split(",") if ident else []
    sekarang = parts[1] if len(parts) > 1 else None
    rows = []
    for urut, (arch, scen) in enumerate(URUTAN_SESI, 1):
        row = {"arch": arch, "scen": scen, "masalah": ""}
        try:
            tunggu(f"[{urut}/{len(URUTAN_SESI)}] {arch} skenario {scen} - tekan Enter untuk mulai ")
            if sekarang != arch:
                ganti_mode(lg, arch, stlink, out=out, flash=flash, wait=wait)
                sekarang = arch
            on_start = on_start_factory(scen) if on_start_factory else None
            raw, res, bad = jalankan_run(lg, arch, scen, secs, hitung, jeda, out=out, sleep=sleep,
                                         on_start=on_start, analisis=analisis, rshunt=rshunt, offset=offset)
            row.update(res)
            row["file"] = simpan_run(folder, urut, arch, scen, raw).name
            row["masalah"] = "; ".join(bad)
            out(run_bench.fmt(res, bad) if res else "")
        except (HostError, TimeoutError, RuntimeError, FileNotFoundError) as e:
            row["masalah"] = str(e) or type(e).__name__
            out(f"RUN GAGAL: {row['masalah']}")
        out("Ketik x di serial monitor attacker.")
        rows.append(row)
    return rows


# --------------------------------------------------------------------------- CLI
MODE = {"k1": "K1S", "k2": "K2S"}
BANTUAN = """perintah:
  ping          tanya host (arsitektur, siap)
  mode k1|k2    flash host ke K1S (software) atau K2S (hardware offloading) lewat ST-Link
  run A|B       satu run 60 s (A = 1 permintaan/s, B = barrage 10/s) dengan arsitektur host sekarang
  sesi          empat run: K1S.A, K1S.B, (flash) K2S.A, K2S.B
  ringkas       tabel ringkasan sesi terakhir
  keluar"""


def periksa_elf(archs):
    return [f"{a}: {run_bench.ELF[a]} belum dibangun (pio run -e {run_bench.ELF[a].parent.name})"
            for a in archs if not run_bench.ELF[a].exists()]


def fabrik_attacker(at):
    """on_start_factory untuk --auto-attacker: kirim a/b tepat setelah >START (prosedur pengambilan data)."""
    return lambda scen: (lambda: at.send("a" if scen == "A" else "b"))


def buat_parser():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--logger", default="COM18", help="port USB CDC logger")
    ap.add_argument("--stlink", default="$", help="nomor seri ST-Link host (untuk flash)")
    ap.add_argument("--secs", type=int, default=60)
    ap.add_argument("--hitung", type=int, default=5, help="hitung mundur sebelum BISA MULAI MENYERANG (s)")
    ap.add_argument("--jeda", type=int, default=5, help="jeda setelah BISA MULAI MENYERANG sebelum jendela host (s)")
    ap.add_argument("--rshunt", type=float, default=kp.RSHUNT_EFEKTIF)     # kalibrasi 21 Nov 2026 (nominal modul 0,100)
    ap.add_argument("--offset", type=float, default=kp.OFFSET_LSB)
    ap.add_argument("--auto-attacker", metavar="PORT", help="CADANGAN: skrip memulai attacker setelah >START "
                    "(serial monitor attacker harus ditutup)")
    ap.add_argument("--kering", action="store_true", help="cetak urutan sesi saja, tanpa perangkat")
    return ap


def _catatan_sesi(ident, args):
    elf = "".join(f"elf_{a}={run_bench.ELF[a]} sha256={run_bench.sha_short(run_bench.ELF[a])}\n"
                  for a in ("K1S", "K2S") if run_bench.ELF[a].exists())
    return (f"host={ident}\nsecs={args.secs}\nrshunt={args.rshunt}\noffset={args.offset}\n"
            f"hitung={args.hitung}\njeda={args.jeda}\nauto_attacker={args.auto_attacker}\n"
            f"{elf}{CATATAN_STLINK}\n")


def lingkar(lg, args, at, masukan=input, out=print, akar_demo=ROOT / "data" / "demo"):
    out(BANTUAN)
    rows = []
    fab = fabrik_attacker(at) if at else None
    while True:
        try:
            baris = masukan("demo> ").strip().split()
        except EOFError:
            return
        if not baris:
            continue
        kmd, arg = baris[0].lower(), (baris[1] if len(baris) > 1 else "")
        try:
            if kmd == "keluar":
                return
            elif kmd == "bantuan":
                out(BANTUAN)
            elif kmd == "ping":
                out(f"host: {run_bench.host_id(lg) or 'TIDAK MENJAWAB'}")
            elif kmd == "mode":
                if arg.lower() not in MODE:
                    out("pilih mode k1 atau k2")
                else:
                    ganti_mode(lg, MODE[arg.lower()], args.stlink, out=out)
            elif kmd == "run":
                scen = arg.upper()
                if scen not in ("A", "B"):
                    out("pakai: run A  atau  run B")
                    continue
                ident = run_bench.host_id(lg)
                if ident is None:
                    raise HostTidakMenjawab("host tidak menjawab '?' (periksa kabel logger PA9 -> host PA10, daya host)")
                arch = ident.split(",")[1]
                raw, res, bad = jalankan_run(lg, arch, scen, args.secs, args.hitung, args.jeda, out=out,
                                             on_start=fab(scen) if fab else None, rshunt=args.rshunt,
                                             offset=args.offset)
                folder = Path(akar_demo) / f"{datetime.now():%Y%m%d_%H%M%S}_{arch}_{scen}"
                folder.mkdir(parents=True, exist_ok=True)
                out(f"disimpan: {simpan_run(folder, 1, arch, scen, raw)}")
                out(run_bench.fmt(res, bad))
                out("Ketik x di serial monitor attacker.")
            elif kmd == "sesi":
                folder = Path(akar_demo) / f"{datetime.now():%Y%m%d_%H%M%S}"
                folder.mkdir(parents=True, exist_ok=True)
                (folder / "sesi.txt").write_text(_catatan_sesi(run_bench.host_id(lg), args), encoding="utf-8")
                rows = jalankan_sesi(lg, folder, secs=args.secs, hitung=args.hitung, jeda=args.jeda,
                                     stlink=args.stlink, tunggu=masukan, out=out, on_start_factory=fab,
                                     rshunt=args.rshunt, offset=args.offset)
                out(tabel_ringkas(rows))
                out(CATATAN_STLINK)
                out(f"hasil mentah: {folder}")
            elif kmd == "ringkas":
                if rows:
                    out(tabel_ringkas(rows))
                    out(CATATAN_STLINK)
                else:
                    out("belum ada sesi")
            else:
                out(f"perintah tidak dikenal: {kmd} (ketik bantuan)")
        except (HostError, TimeoutError, RuntimeError, FileNotFoundError) as e:
            out(f"GAGAL: {e}")


def main(argv=None):
    args = buat_parser().parse_args(argv)
    if args.kering:
        ganti = sum(a[0] != b[0] for a, b in zip(URUTAN_SESI, URUTAN_SESI[1:]))
        print(f"{len(URUTAN_SESI)} run x {args.secs} s; ganti arsitektur (flash {ganti} kali di depan penguji)")
        for i, (a, sc) in enumerate(URUTAN_SESI, 1):
            print(f"  {i}. {a} {sc}")
        return 0
    pesan = periksa_elf(["K1S", "K2S"])
    if pesan:
        print("ELF host belum lengkap:\n  " + "\n  ".join(pesan))
        return 2
    try:
        lg = run_bench.Logger(args.logger)
    except serial.SerialException as e:
        print(f"Port logger {args.logger} tidak bisa dibuka ({e}). Periksa nomor COM dan tutup program lain.")
        return 2
    at = None
    try:
        if args.auto_attacker:
            try:
                at = run_bench.Attacker(args.auto_attacker)
            except serial.SerialException as e:
                print(f"Port attacker {args.auto_attacker} tidak bisa dibuka ({e}). "
                      "Tutup serial monitor attacker dulu, atau jalankan tanpa --auto-attacker.")
                return 2
        lingkar(lg, args, at)
    finally:
        lg.close()
        if at:
            at.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
