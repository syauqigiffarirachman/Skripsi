# PlatformIO extra script (SCons): tulis provenance.json di samping firmware.bin setelah build.
#
# STATUS: BELUM PERNAH DIJALANKAN dan BELUM didaftarkan di platformio.ini. Tidak mengubah isi firmware.bin
# (hanya menulis berkas baru di folder build), jadi gerbang `tools/cek_biner.py` tidak terpengaruh.
#
# Cara memakai (lakukan SAAT TIDAK ADA build/flash berjalan: mengubah platformio.ini membuat PlatformIO
# menghapus semua folder build):
#   1. Di platformio.ini, bagian [env] (atau env tertentu), tambahkan:
#        extra_scripts = post:extra_script_provenance.py
#   2. Bangun satu env, mis. `pio run -e host_k2_stop`, lalu periksa
#        .pio/build/host_k2_stop/provenance.json
#   3. Cocokkan: bin_sha256 harus sama dengan `sha256sum .pio/build/host_k2_stop/firmware.bin`.
# Bila target `$BUILD_DIR/${PROGNAME}.bin` tidak ada di platform ini, hook tidak berjalan (tidak ada galat);
# periksa langkah 2 dan sesuaikan nama target.
#
# Logika ada di tools/provenance.py (dapat diuji tanpa PlatformIO: tests/test_provenance.py).
Import("env")  # noqa: F821  (disediakan SCons/PlatformIO)

import sys
from pathlib import Path

sys.path.insert(0, str(Path(env["PROJECT_DIR"]) / "tools"))  # noqa: F821
import provenance  # noqa: E402


def _catat(source, target, env):  # noqa: F811
    try:
        print(f"[provenance] {provenance.tulis(str(target[0]), env['PIOENV'])}")
    except Exception as e:  # catatan tak boleh menggagalkan build
        print(f"[provenance] GAGAL menulis catatan: {e}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _catat)  # noqa: F821
