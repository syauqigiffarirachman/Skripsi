"""Uji jalur galat protokol logger<->host (15 Nov 2026), host K1 tahap 5 (k1_sw).

Galat dipaksa lewat OpenOCD (port TCL 6666) di ST-Link host, tanpa mengubah kabel:
  E1 host di-halt sebelum run       -> ERR_NOSTART (+ ping -> #ERR,host_diam)
  E2 host di-halt 3 s dalam jendela -> ERR_NOEND (lanjut tanpa reset: baris basi harus dibuang; lalu dengan reset)
  E3 breakpoint tepat sebelum >DONE -> ERR_NODONE
  E4 g_ready dinolkan di RAM        -> host membalas >ERR,belum_siap -> ERR_HOST
Setelah tiap galat, satu run normal harus OK lagi.

Prasyarat: server OpenOCD berjalan di ST-Link host:
  openocd -c 'adapter serial {$}' -f interface/stlink.cfg -c "transport select hla_swd" -f target/stm32f1x.cfg -c init
Alamat BP_DONE / G_READY berlaku untuk ELF k1_sw sha256 9b1adf623e29; cari ulang dengan arm-none-eabi-nm (g_ready)
dan objdump (instruksi yang memuat alamat string ">DONE") bila firmware berubah. Setelah uji, hentikan server lalu
jalankan 'run_bench.py nodebug' (server OpenOCD menyalakan DBGMCU_CR = 0x307).
"""
import os
import socket
import sys
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from run_bench import Logger, parse

BP_DONE = 0x080075A4      # k1_sw: tepat sebelum hl_tx(">DONE")
G_READY = 0x20001788      # k1_sw: bool g_ready (HL_READY)
OUT = Path(__file__).resolve().parent.parent / "data" / "dev" / "err"


class Ocd:
    """Klien port TCL OpenOCD."""
    def __init__(self, host="127.0.0.1", port=6666):
        self.s = socket.create_connection((host, port), timeout=10)

    def cmd(self, c):
        self.s.sendall(c.encode() + b"\x1a")
        buf = b""
        while not buf.endswith(b"\x1a"):
            buf += self.s.recv(4096)
        return buf[:-1].decode(errors="replace").strip()


o = Ocd()
lg = Logger(sys.argv[1] if len(sys.argv) > 1 else "COM18")
hasil = []


def run(nama, cmd, secs, harap, on_start=None):
    t0 = time.time()
    seen = []

    def on_line(l):
        if not seen and l.startswith(b"h,") and b">START" in l:
            seen.append(time.time())
            if on_start:
                on_start()
    try:
        raw = lg.command(cmd, secs, secs + 25, on_line)
    except Exception as ex:
        print(f"[{nama}] PENGECUALIAN {ex}")
        hasil.append((nama, harap, f"exc:{ex}", False))
        return None
    os.makedirs(OUT, exist_ok=True)
    (OUT / f"{nama}.csv").write_bytes(raw)
    r = parse(raw.decode(errors="replace"))
    ok = r["status"] == harap
    hasil.append((nama, harap, r["status"], ok))
    print(f"[{nama}] status={r['status']} (harap {harap}) {'LULUS' if ok else 'GAGAL'}  lama={time.time()-t0:.1f}s  "
          f"s={len(r['s'])} e={len(r['e'])} h={[h[1][:60] for h in r['h']]}", flush=True)
    return r


def ping(nama, harap_ada):
    raw = lg.command(3, 0, 5).decode(errors="replace")
    idl = [l for l in raw.splitlines() if ">ID" in l]
    errl = [l for l in raw.splitlines() if l.startswith("#ERR")]
    ok = bool(idl) == harap_ada
    hasil.append((nama, "ID" if harap_ada else "#ERR,host_diam", (idl or errl or ["-"])[0], ok))
    print(f"[{nama}] {(idl or errl or ['(kosong)'])[0]}  {'LULUS' if ok else 'GAGAL'}", flush=True)


def tunggu_host(maks=15):
    t = time.time()
    while time.time() - t < maks:
        if ">ID" in lg.command(3, 0, 5).decode(errors="replace"):
            return time.time() - t
        time.sleep(0.5)
    return None


def halt_nanti(detik):
    return lambda: threading.Timer(detik, lambda: Ocd().cmd("halt")).start()


run("E0_dasar", 5, 5, "OK")
# E1 host berhenti sebelum run
o.cmd("halt")
ping("E1a_ping_host_henti", False)
run("E1b_nostart", 5, 5, "ERR_NOSTART")
o.cmd("resume")
ping("E1c_ping_setelah_lanjut", True)
run("E1d_pulih", 5, 5, "OK")
# E2 host berhenti di tengah jendela
run("E2a_noend", 5, 10, "ERR_NOEND", on_start=halt_nanti(3.0))
o.cmd("resume")
time.sleep(10)                                     # host menuntaskan jendela lama, mengirim >END/>DONE basi
run("E2b_setelah_basi", 5, 5, "OK")
run("E2c_noend", 5, 10, "ERR_NOEND", on_start=halt_nanti(3.0))
o.cmd("reset run")
print(f"  host kembali menjawab setelah reset: {tunggu_host()} s")
run("E2d_pulih_reset", 5, 5, "OK")
# E3 host berhenti tepat sebelum >DONE
o.cmd(f"bp 0x{BP_DONE:08x} 2 hw")
run("E3a_nodone", 5, 5, "ERR_NODONE")
print("  pc saat henti:", o.cmd("reg pc"))
o.cmd(f"rbp 0x{BP_DONE:08x}")
o.cmd("resume")
time.sleep(1)
run("E3b_pulih", 5, 5, "OK")
# E4 host menolak (belum siap)
o.cmd(f"mwb 0x{G_READY:08x} 0")
run("E4a_host_tolak", 5, 5, "ERR_HOST")
o.cmd(f"mwb 0x{G_READY:08x} 1")
run("E4b_pulih", 5, 5, "OK")
lg.close()

print("\nRINGKASAN")
for n, h, s, ok in hasil:
    print(f"  {'LULUS' if ok else 'GAGAL'}  {n:24s} harap={h:16s} dapat={s}")
print(f"lulus {sum(1 for x in hasil if x[3])}/{len(hasil)}")
