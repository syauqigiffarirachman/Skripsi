"""Pulihkan host F103 yang terkunci (SWD dimatikan firmware) lewat BOOT0 (18 Nov 2026).

Syarat: BOOT0 = 1 (jumper Blue Pill), lalu cabut-pasang daya (POR) supaya chip menjalankan bootloader
sistem (SWD hidup di sana). Skrip: halt, hapus total flash, tulis firmware, verifikasi.
Setelah selesai: BOOT0 kembali 0 lalu cabut-pasang daya.
  python tools/dev/tahap_stop/pulihkan_host.py [firmware.elf]     # bawaan: .pio/build/host_s2_rx/firmware.elf
"""
import subprocess
import sys
from pathlib import Path

OCD = Path.home() / ".platformio/packages/tool-openocd"
elf = Path(sys.argv[1] if len(sys.argv) > 1 else ".pio/build/host_s2_rx/firmware.elf").resolve()
cmd = [str(OCD / "bin/openocd.exe"), "-s", str(OCD / "openocd/scripts"),
       "-f", "interface/stlink.cfg", "-c", "transport select hla_swd", "-f", "target/stm32f1x.cfg",
       "-c", "reset_config none", "-c", "init", "-c", "halt", "-c", "stm32f1x mass_erase 0",
       "-c", f"program {{{elf.as_posix()}}} verify", "-c", "shutdown"]
r = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
print((r.stdout + r.stderr)[-1500:])
print("OK" if r.returncode == 0 else "GAGAL (cek BOOT0=1 dan POR)")
sys.exit(r.returncode)
