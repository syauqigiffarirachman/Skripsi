"""Uji tools/provenance.py. Memakai repo git SEMENTARA di folder temp; repo proyek tidak disentuh.

Jalankan: .venv-analisis/Scripts/python -B -m unittest discover -s tests -v
"""
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import provenance  # noqa: E402

GIT = shutil.which("git")


def git(cwd, *a):
    # commit.gpgsign=false: repo sementara tidak punya kunci penandatangan
    return subprocess.run([GIT, "-c", "user.name=uji", "-c", "user.email=uji@example.invalid",
                           "-c", "commit.gpgsign=false", *a], cwd=cwd, capture_output=True, text=True, check=True)


class TestKumpulkan(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="uji_prov_"))
        self.addCleanup(shutil.rmtree, self.tmp, True)
        self.bin = self.tmp / "firmware.bin"
        self.bin.write_bytes(bytes(range(256)) * 4)

    def test_hash_dan_ukuran_cocok(self):
        c = provenance.kumpulkan(self.bin, "uji", root=self.tmp / "bukan_repo")
        self.assertEqual(c["bin_sha256"], hashlib.sha256(self.bin.read_bytes()).hexdigest())
        self.assertEqual(c["bin_ukuran"], 1024)
        self.assertEqual(c["env"], "uji")
        self.assertIsNone(c["git_head"])                 # bukan repo / folder tak ada
        self.assertIsNone(c["dirty"])                    # tidak bisa ditentukan, BUKAN False

    def test_tulis_tidak_mengubah_biner(self):
        sebelum = self.bin.read_bytes()
        p = provenance.tulis(self.bin, "uji", root=self.tmp / "bukan_repo")
        self.assertEqual(p, self.tmp / "provenance.json")
        self.assertEqual(self.bin.read_bytes(), sebelum)
        self.assertEqual(json.loads(p.read_text(encoding="utf-8"))["env"], "uji")
        self.assertFalse((self.tmp / "provenance.json.tmp").exists())

    def test_git_di_folder_tak_ada_mengembalikan_none(self):
        self.assertIsNone(provenance._git("rev-parse", "HEAD", cwd=self.tmp / "tidak_ada"))

    def test_baca_berkas_rusak_atau_tak_ada(self):
        self.assertIsNone(provenance.baca("tidak_ada", root=self.tmp))
        d = self.tmp / ".pio" / "build" / "x"
        d.mkdir(parents=True)
        (d / "provenance.json").write_text("{rusak", encoding="utf-8")
        self.assertIsNone(provenance.baca("x", root=self.tmp))

    @unittest.skipUnless(GIT, "git tidak tersedia")
    def test_dirty_mengikuti_perubahan_sumber(self):
        repo = self.tmp / "repo"
        (repo / "src").mkdir(parents=True)
        (repo / "src" / "a.cpp").write_text("int a;\n", encoding="utf-8")
        git(repo, "init", "-q")
        git(repo, "add", ".")
        git(repo, "commit", "-q", "-m", "awal")
        bersih = provenance.kumpulkan(self.bin, "uji", root=repo)
        self.assertFalse(bersih["dirty"])
        self.assertEqual(len(bersih["git_head"]), 40)
        (repo / "src" / "a.cpp").write_text("int a; // diubah\n", encoding="utf-8")
        kotor = provenance.kumpulkan(self.bin, "uji", root=repo)
        self.assertTrue(kotor["dirty"])
        self.assertEqual(kotor["berkas_berubah"], ["src/a.cpp"])
        self.assertEqual(kotor["berkas_berubah_total"], 1)

    @unittest.skipUnless(GIT, "git tidak tersedia")
    def test_berkas_di_luar_jalur_sumber_tidak_membuat_kotor(self):
        repo = self.tmp / "repo2"
        (repo / "src").mkdir(parents=True)
        (repo / "src" / "a.cpp").write_text("int a;\n", encoding="utf-8")
        git(repo, "init", "-q")
        git(repo, "add", ".")
        git(repo, "commit", "-q", "-m", "awal")
        (repo / "catatan.md").write_text("bukan sumber firmware\n", encoding="utf-8")
        self.assertFalse(provenance.kumpulkan(self.bin, "uji", root=repo)["dirty"])


if __name__ == "__main__":
    unittest.main()
