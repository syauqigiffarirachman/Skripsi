"""Uji pemisahan FINAL vs DEV (24 Nov 2026): kode final tidak boleh bergantung pada folder dev/ (lihat
tools/4_verifikasi/cek_pemisahan.py). Statis, tanpa perangkat keras dan tanpa membangun."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import _jalur  # noqa: E402,F401  (semua subfolder tools/ masuk sys.path)
import cek_pemisahan  # noqa: E402


class PemisahanFinalDev(unittest.TestCase):
    def test_firmware_final_tidak_memuat_dev(self):
        masalah, dilihat = cek_pemisahan.cek_firmware()
        self.assertGreater(dilihat, 10)          # pemeriksaan benar-benar menelusuri berkas
        self.assertEqual(masalah, [])

    def test_skrip_final_tidak_mengimpor_dev(self):
        masalah, dilihat = cek_pemisahan.cek_skrip()
        self.assertGreater(dilihat, 10)
        self.assertEqual(masalah, [])


if __name__ == "__main__":
    unittest.main()
