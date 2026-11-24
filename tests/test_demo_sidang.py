"""Uji tools/demo_sidang.py (demo sidang) tanpa perangkat: logika murni dan orkestrasi dengan logger PALSU.

Jalankan: .venv-analisis/Scripts/python -B -m unittest discover -s tests -v
Tidak membuka port serial, tidak menyentuh .pio/ atau perangkat keras.
"""
import io
import re
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
try:
    import run_bench as rb
    import demo_sidang as ds
except ModuleNotFoundError as e:
    if e.name in ("serial", "run_bench"):
        raise unittest.SkipTest(f"tak bisa diimpor: {e}")
    raise

TICK = rb.TICK_HZ


# --------------------------------------------------------------------------- Task 1: logika murni
class TestLogikaMurni(unittest.TestCase):
    def test_urutan_sesi_satu_flash(self):
        ganti = sum(a[0] != b[0] for a, b in zip(ds.URUTAN_SESI, ds.URUTAN_SESI[1:]))
        self.assertEqual(ganti, 1)
        self.assertEqual(ds.URUTAN_SESI, [("K1S", "A"), ("K1S", "B"), ("K2S", "A"), ("K2S", "B")])

    def test_livemeter_arus_per_detik(self):
        # shunt mentah 84, offset -2 -> 84 - (-2) = 86 LSB; 86 x 2,5 uV / 0,100 Ohm = 2,15 mA (dihitung tangan)
        m = ds.LiveMeter(rshunt=0.100, offset=-2.0)
        keluar = []
        for k in range(1001):                              # 500 cuplikan/s selama 2 s + 1 cuplikan di detik ke-2
            keluar += m.feed(f"s,{k * TICK // 500},84,0".encode())
        tampil = [x for x in keluar if "I=" in x]
        self.assertEqual(len(tampil), 2)
        for x in tampil:
            mA = float(re.search(r"I=\s*([0-9.]+)", x).group(1))
            self.assertAlmostEqual(mA, 2.15, places=2)
        self.assertIn("t=  0", tampil[0])
        self.assertIn("t=  1", tampil[1])

    def test_livemeter_baris_host_penting_saja(self):
        m = ds.LiveMeter()
        self.assertTrue(any(">START" in x for x in m.feed(b"h,100,>START,60")))
        self.assertEqual(m.feed(b"h,200,>L,1,2,3"), [])
        self.assertTrue(any(">END" in x for x in m.feed(b"h,300,>END,K1S,60")))
        self.assertTrue(any(">DONE" in x for x in m.feed(b"h,400,>DONE")))

    def test_tabel_ringkas_empat_baris(self):
        def baris(arch, scen, **kw):
            d = dict(arch=arch, scen=scen, I_avg_mA=2.0, arrived=60, served=60, N_rusak=0, N_lwt=0,
                     lat_mean_ms_nyata=116.2, TL_pct=100.0)
            d.update(kw)
            return d
        rows = [baris("K1S", "A"), baris("K1S", "B", masalah="NOSTART"), baris("K2S", "A"), {"arch": "K2S", "scen": "B"}]
        teks = ds.tabel_ringkas(rows)
        for kata in ("K1S", "K2S", " A", " B"):
            self.assertIn(kata, teks)
        gagal = [l for l in teks.splitlines() if "GAGAL" in l]
        self.assertEqual(len(gagal), 1)
        self.assertIn("K1S", gagal[0])

    def test_tabel_run_selesai_dengan_catatan_bukan_gagal(self):
        # run selesai (ada berkas + hasil) tetapi di luar toleransi: label PERIKSA, bukan GAGAL (jangan menyesatkan penguji)
        r = dict(arch="K2S", scen="B", I_avg_mA=7.4, arrived=601, served=515, N_rusak=2, N_lwt=86,
                 lat_mean_ms_nyata=116.3, TL_pct=85.7, file="004_K2S_B.csv", masalah="host: hilang=2")
        teks = ds.tabel_ringkas([r])
        self.assertNotIn("GAGAL", teks)
        self.assertIn("PERIKSA", teks)
        self.assertIn("hilang=2", teks)

    def test_simpan_run_nama_cocok_ringkas(self):
        with tempfile.TemporaryDirectory() as d:
            p = ds.simpan_run(Path(d), 3, "K2S", "B", b"x")
            self.assertEqual(p.name, "003_K2S_B.csv")
            self.assertEqual(p.stem.split("_", 2)[2], "B")
            self.assertEqual(p.read_bytes(), b"x")


# --------------------------------------------------------------------------- Task 2: orkestrasi (logger palsu)
class _Port:
    baudrate = 115200


class FakeLogger:
    """Meniru Logger.command: cmd 3 -> baris >ID sesuai arsitektur host; cmd 4 -> >START lalu CSV sintetis."""

    def __init__(self, arch="K1S", ada_id=True, gagal_run=(), ctrl_c=False):
        self.s = _Port()
        self.arch, self.ada_id, self.gagal_run, self.ctrl_c = arch, ada_id, set(gagal_run), ctrl_c
        self.calls = []
        self.run_ke = 0

    def close(self):
        pass

    def command(self, cmd, arg, timeout, on_line=None):
        self.calls.append((cmd, arg))
        if cmd == 3:
            idl = f"h,1,>ID,{self.arch},8,48000000,1\r\n".encode() if self.ada_id else b""
            return idl + b"#CMD,SELESAI\r\n"
        self.run_ke += 1
        self.s.baudrate = 1_000_000 + cmd * 1000 + arg            # seperti Logger asli
        if self.ctrl_c:
            raise KeyboardInterrupt
        if self.run_ke in self.gagal_run:
            raise TimeoutError("logger tidak menyelesaikan perintah")
        if on_line:
            on_line(b"h,5,>START,60")
        self.s.baudrate = 115200
        return f"#RUN,BEGIN,{self.run_ke}\r\ns,0,84,0\r\n#CMD,SELESAI\r\n".encode()


def analisis_palsu(text, rshunt, offset):
    return {"I_avg_mA": 2.0, "arrived": 60, "served": 60, "N_rusak": 0}, []


class TestOrkestrasi(unittest.TestCase):
    def test_urutan_banner_dan_perintah(self):
        lg, out, tidur = FakeLogger("K1S"), [], []
        raw, res, bad = ds.jalankan_run(lg, "K1S", "A", 60, 5, 5, out=out.append, sleep=tidur.append,
                                        analisis=analisis_palsu)
        i_siap = next(i for i, x in enumerate(out) if "HOST SIAP" in x)
        i_mulai = next(i for i, x in enumerate(out) if "BISA MULAI MENYERANG" in x)
        self.assertLess(i_siap, i_mulai)
        self.assertEqual(lg.calls, [(3, 0), (4, 60)])
        self.assertEqual(tidur, [1, 1, 1, 1, 1, 5])
        self.assertEqual(res["N_rusak"], 0)
        self.assertEqual(bad, [])
        self.assertIn(b"#RUN,BEGIN", raw)

    def test_petunjuk_attacker_sesuai_skenario(self):
        for scen, huruf in (("A", "a"), ("B", "b")):
            out = []
            ds.jalankan_run(FakeLogger("K1S"), "K1S", scen, 60, 0, 0, out=out.append, sleep=lambda s: None,
                            analisis=analisis_palsu)
            mulai = next(x for x in out if "BISA MULAI MENYERANG" in x)
            self.assertIn(f"ketik {huruf}", mulai)

    def test_host_tidak_menjawab_tidak_kirim_run(self):
        lg = FakeLogger("K1S", ada_id=False)
        with self.assertRaises(ds.HostTidakMenjawab):
            ds.jalankan_run(lg, "K1S", "A", 60, 5, 5, out=lambda x: None, sleep=lambda s: None,
                            analisis=analisis_palsu)
        self.assertNotIn(4, [c[0] for c in lg.calls])

    def test_host_salah_arsitektur(self):
        lg = FakeLogger("K2S")
        with self.assertRaises(ds.HostSalahArsitektur) as cm:
            ds.jalankan_run(lg, "K1S", "A", 60, 5, 5, out=lambda x: None, sleep=lambda s: None,
                            analisis=analisis_palsu)
        self.assertIn("mode k1", str(cm.exception))
        self.assertNotIn(4, [c[0] for c in lg.calls])

    def test_baud_dikembalikan_saat_ctrl_c(self):
        lg = FakeLogger("K1S", ctrl_c=True)
        with self.assertRaises(KeyboardInterrupt):
            ds.jalankan_run(lg, "K1S", "A", 60, 0, 0, out=lambda x: None, sleep=lambda s: None,
                            analisis=analisis_palsu)
        self.assertEqual(lg.s.baudrate, 115200)

    def test_on_start_dipanggil_sekali_saat_start(self):
        n = []
        ds.jalankan_run(FakeLogger("K1S"), "K1S", "B", 60, 0, 0, out=lambda x: None, sleep=lambda s: None,
                        on_start=lambda: n.append(1), analisis=analisis_palsu)
        self.assertEqual(n, [1])

    def test_ganti_mode_flash_lalu_tunggu(self):
        lg, urut = FakeLogger("K1S"), []
        ident = ds.ganti_mode(lg, "K2S", "$", out=lambda x: None,
                              flash=lambda st, a: urut.append(("flash", st, a)),
                              wait=lambda l, a, m=25: (urut.append(("wait", a)) or f">ID,{a},8,48000000,1"))
        self.assertEqual(urut, [("flash", "$", "K2S"), ("wait", "K2S")])
        self.assertIn("K2S", ident)
        with self.assertRaises(ds.HostTidakMenjawab):
            ds.ganti_mode(lg, "K2S", "$", out=lambda x: None, flash=lambda st, a: None, wait=lambda l, a, m=25: None)

    def test_sesi_lanjut_bila_satu_run_gagal(self):
        lg, tunggu, flash_dipanggil = FakeLogger("K1S", gagal_run=[2]), [], []

        def flash(stlink, arch):
            flash_dipanggil.append(arch)
            lg.arch = arch

        with tempfile.TemporaryDirectory() as d:
            rows = ds.jalankan_sesi(lg, Path(d), secs=60, hitung=0, jeda=0, tunggu=tunggu.append,
                                    out=lambda x: None, sleep=lambda s: None, flash=flash,
                                    wait=lambda l, a, m=25: f">ID,{a},8,48000000,1", analisis=analisis_palsu)
            berkas = sorted(p.name for p in Path(d).glob("[0-9][0-9][0-9]_*.csv"))
        self.assertEqual(len(rows), 4)
        self.assertTrue(rows[1]["masalah"])
        self.assertFalse(rows[0]["masalah"] or rows[2]["masalah"] or rows[3]["masalah"])
        self.assertEqual(flash_dipanggil, ["K2S"])
        self.assertEqual(len(tunggu), 4)
        self.assertEqual(berkas, ["001_K1S_A.csv", "003_K2S_A.csv", "004_K2S_B.csv"])
        self.assertEqual([r["arch"] + r["scen"] for r in rows], ["K1SA", "K1SB", "K2SA", "K2SB"])


# --------------------------------------------------------------------------- Task 3: CLI
class _AtPalsu:
    def __init__(self):
        self.terkirim = []

    def send(self, ch):
        self.terkirim.append(ch)

    def close(self):
        pass


def _args(*extra):
    return ds.buat_parser().parse_args(["--hitung", "0", "--jeda", "0", *extra])


class TestCLI(unittest.TestCase):
    def _lingkar(self, lg, perintah, at=None, akar=None, **kw):
        out = []
        it = iter(perintah)
        ds.lingkar(lg, _args(), at, masukan=lambda p="": next(it), out=out.append,
                   akar_demo=akar or Path(tempfile.mkdtemp()), **kw)
        return "\n".join(out)

    def test_periksa_elf_melaporkan_yang_hilang(self):
        with tempfile.TemporaryDirectory() as d:
            ada = Path(d) / "firmware.elf"
            ada.write_bytes(b"x")
            hilang = Path(d) / "host_k1_stop" / "firmware.elf"
            with mock.patch.dict(rb.ELF, {"K1S": hilang, "K2S": ada}):
                pesan = ds.periksa_elf(["K1S", "K2S"])
            self.assertEqual(len(pesan), 1)
            self.assertIn("K1S", pesan[0])
            self.assertIn("pio run -e host_k1_stop", pesan[0])
            with mock.patch.dict(rb.ELF, {"K1S": ada, "K2S": ada}):
                self.assertEqual(ds.periksa_elf(["K1S", "K2S"]), [])

    def test_kering_mencetak_urutan_tanpa_perangkat(self):
        buf = io.StringIO()
        with mock.patch.object(rb, "Logger", side_effect=AssertionError("tidak boleh membuka port")), \
                redirect_stdout(buf):
            kode = ds.main(["--kering"])
        self.assertEqual(kode, 0)
        teks = buf.getvalue()
        for baris in ("K1S A", "K1S B", "K2S A", "K2S B"):
            self.assertIn(baris, teks)
        self.assertIn("flash 1 kali", teks)

    def test_auto_attacker_port_terpakai_pesan_ramah(self):
        import serial
        buf = io.StringIO()
        with mock.patch.object(ds, "periksa_elf", return_value=[]), \
                mock.patch.object(rb, "Logger", return_value=FakeLogger("K1S")), \
                mock.patch.object(rb, "Attacker", side_effect=serial.SerialException("PermissionError")), \
                redirect_stdout(buf):
            kode = ds.main(["--logger", "X", "--auto-attacker", "COM99"])
        self.assertNotEqual(kode, 0)
        self.assertIn("serial monitor", buf.getvalue())
        self.assertNotIn("Traceback", buf.getvalue())

    def test_logger_port_gagal_pesan_ramah(self):
        import serial
        buf = io.StringIO()
        with mock.patch.object(ds, "periksa_elf", return_value=[]), \
                mock.patch.object(rb, "Logger", side_effect=serial.SerialException("could not open")), \
                redirect_stdout(buf):
            kode = ds.main(["--logger", "COM77"])
        self.assertNotEqual(kode, 0)
        self.assertIn("COM77", buf.getvalue())

    def test_fabrik_attacker_kirim_huruf_sesuai_skenario(self):
        at = _AtPalsu()
        ds.fabrik_attacker(at)("A")()
        ds.fabrik_attacker(at)("B")()
        self.assertEqual(at.terkirim, ["a", "b"])

    def test_lingkar_bantuan_dan_tak_dikenal(self):
        teks = self._lingkar(FakeLogger("K1S"), ["bantuan", "ngawur", "keluar"])
        self.assertIn("sesi", teks)
        self.assertIn("tidak dikenal", teks)

    def test_lingkar_mode_salah_dan_ringkas_kosong(self):
        teks = self._lingkar(FakeLogger("K1S"), ["mode zz", "ringkas", "keluar"])
        self.assertIn("k1 atau k2", teks)
        self.assertIn("belum ada sesi", teks)

    def test_lingkar_run_host_bisu_tidak_crash(self):
        teks = self._lingkar(FakeLogger("K1S", ada_id=False), ["run A", "keluar"])
        self.assertIn("tidak menjawab", teks)

    def test_lingkar_run_memakai_arsitektur_host(self):
        lg = FakeLogger("K2S")
        teks = self._lingkar(lg, ["run B", "keluar"])
        self.assertIn("HOST SIAP (K2S)", teks)
        self.assertEqual([c for c in lg.calls if c[0] == 4], [(4, 60)])

    def test_lingkar_sesi_menulis_catatan_dan_tabel(self):
        baris = [{"arch": "K1S", "scen": "A", "I_avg_mA": 2.0, "masalah": ""}]
        with tempfile.TemporaryDirectory() as d, \
                mock.patch.object(ds, "jalankan_sesi", return_value=baris) as js:
            teks = self._lingkar(FakeLogger("K1S"), ["sesi", "ringkas", "keluar"], akar=Path(d))
            folder = js.call_args[0][1]
            catatan = (Path(folder) / "sesi.txt").read_text(encoding="utf-8")
            self.assertEqual(Path(folder).parent, Path(d))
        # sesi.txt mencatat kalibrasi yang dipakai: bawaan CLI = hasil kalibrasi 21 Nov 2026 (tools/kalibrasi_param.py)
        self.assertIn(f"rshunt={ds.kp.RSHUNT_EFEKTIF}", catatan)
        self.assertIn(f"offset={ds.kp.OFFSET_LSB}", catatan)
        self.assertIn("ST-Link", catatan)
        self.assertIn("K1S", teks)
        self.assertIn(ds.CATATAN_STLINK, teks)


if __name__ == "__main__":
    unittest.main()
