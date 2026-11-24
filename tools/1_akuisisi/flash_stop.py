"""Flash / baca-keadaan host yang sedang di Stop mode (17 Nov 2026), tanpa menekan RESET.

Selama Stop SWD tak bisa tersambung dan NRST host tidak ke ST-Link. Skrip ini menjalankan OpenOCD berulang
(satu sesi mencoba memeriksa target tiap beberapa ms sampai inti kebetulan sedang bangun), lalu HALT dan
flash / baca memori. Firmware yang bangun >= tiap 1 s (alarm RTC atau frame attacker) tertangkap dalam
beberapa percobaan. Pemakaian:
  python tools/1_akuisisi/flash_stop.py .pio/build/host_s2_rx/firmware.elf            # flash + reset run
  python tools/1_akuisisi/flash_stop.py --baca 0x20000eb4:12 --baca 0x40002800:8       # baca kata 32-bit (tanpa flash)
  python tools/1_akuisisi/flash_stop.py --baca ... --nama g_rx_ok=firmware.elf         # (opsional) -> alamat dari ELF
Setelah flash: DBGMCU_CR = 0x307 (DBG_STOP menahan Stop) sampai firmware menolkannya (jendela aman 5 s) atau POR.
Inti dibiarkan JALAN (resume) saat selesai kecuali --halt.
"""
import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

OCD = Path.home() / ".platformio/packages/tool-openocd"
NM = Path.home() / ".platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-nm.exe"


def symbols(elf):
    out = subprocess.run([str(NM), str(elf)], capture_output=True, text=True).stdout
    return {l.split()[2]: int(l.split()[0], 16) for l in out.splitlines() if len(l.split()) == 3}


def tcl(elf, reads, ms, halt):
    lines = ["catch {init}", "set ok 0", "set t0 [clock milliseconds]",
             "while {[expr {[clock milliseconds]-$t0}] < %d} {" % ms,
             "  if {![catch {stm32f1x.cpu arp_examine} e]} { if {![catch {halt 100} e2]} { set ok 1; break } }", "}",
             'puts "CATCH_OK=$ok"', "if {$ok} {"]
    lines.append('  puts "PC=[reg pc]"')
    for addr, n in reads:
        lines.append(f'  puts "MEM 0x{addr:08x}: [read_memory 0x{addr:08x} 32 {n}]"')
    if elf:
        lines.append(f"  program {{{Path(elf).as_posix()}}} verify")
        lines.append("  reset run")
    elif not halt:
        lines.append("  resume")
    lines.append("}")
    lines.append("shutdown")
    return "\n".join(lines) + "\n"


def run(stlink, script, timeout=90):
    a = [str(OCD / "bin/openocd"), "-s", str(OCD / "openocd/scripts"), "-c", f"adapter serial {{{stlink}}}",
         "-f", "interface/stlink.cfg", "-c", "transport select hla_swd", "-f", "target/stm32f1x.cfg", "-f", script]
    p = subprocess.run(a, capture_output=True, text=True, timeout=timeout)
    return p.stdout + p.stderr


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("elf", nargs="?")
    ap.add_argument("--stlink", default="$")
    ap.add_argument("--baca", action="append", default=[], help="ALAMAT:JUMLAH_KATA, boleh berulang")
    ap.add_argument("--sym", help="ELF untuk nama simbol (dipakai --baca nama:jumlah)")
    ap.add_argument("--coba", type=int, default=30)
    ap.add_argument("--ms", type=int, default=12000, help="lama satu sesi mencoba (ms)")
    ap.add_argument("--halt", action="store_true")
    a = ap.parse_args()
    sy = symbols(a.sym or a.elf) if (a.sym or a.elf) else {}
    reads = []
    for r in a.baca:
        n, c = r.rsplit(":", 1)
        reads.append((sy[n] if n in sy else int(n, 0), int(c)))
    with tempfile.TemporaryDirectory() as d:
        f = Path(d) / "c.tcl"
        f.write_text(tcl(a.elf, reads, a.ms, a.halt))
        for i in range(1, a.coba + 1):
            o = run(a.stlink, str(f).replace("\\", "/"))
            if "CATCH_OK=1" in o:
                print(f"tertangkap pada percobaan {i}")
                for l in o.splitlines():
                    if l.startswith(("MEM", "PC=")) or "Verified OK" in l or "Programming" in l:
                        print(l)
                if a.elf and "Verified OK" not in o:
                    print(o[-800:])
                    sys.exit(2)
                return
    print("gagal menangkap inti dalam", a.coba, "percobaan")
    sys.exit(1)


if __name__ == "__main__":
    main()
