"""Uji metrik tools/run_bench.py dengan data SINTETIS yang jawabannya bisa dihitung tangan.

Jalankan (tanpa pytest; hanya pustaka standar + pyserial yang sudah ada di .venv-analisis):
    .venv-analisis/Scripts/python -B -m unittest discover -s tests -v

Aturan menulis uji di sini: hitung nilai harapan LEBIH DULU dengan kalkulator, tulis sebagai angka tetap,
baru jalankan. Jangan menyalin keluaran fungsi sebagai nilai harapan (itu menguji diri sendiri).
Uji ini tidak membuka port serial dan tidak menyentuh .pio/ atau perangkat keras.
"""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
try:
    import run_bench as rb
except ModuleNotFoundError as e:                       # mis. pyserial belum terpasang
    raise unittest.SkipTest(f"run_bench tak bisa diimpor: {e}")

TICK = rb.TICK_HZ                                      # 72.000.000


# --------------------------------------------------------------------------- pembangun log sintetis
def buat_log(sh=402, bus=4000, status="OK", host_scale=1.0, lat_cyc=9_600_000, late=0, dt_max=144_000,
             t_sleep_us=None, tanpa_sum=False, cuplikan_hilang=False, bus_rusak=False, tanpa_tepi=False):
    """Satu run sintetis 1,000 s menurut logger.

    Tepi PB1 (tik logger): naik 0 s, turun 0,2 s, naik 0,5 s, turun 1,0 s (relatif ke t0 = 3.600.000 tik)
    -> T_obs = 1,000 s, LOW = 0,3 s, S_cpu = 30,0 %.
    INA226: tiap 144.000 tik (2 ms), 556 cuplikan, nilai tetap sh/bus (arus dan tegangan konstan).
    Host: 2 permintaan datang, 2 dilayani; 2 sign x 9.600.000 siklus @ 48 MHz = 200 ms per sign.
    host_scale = (jam host yang terbaca oleh host) / (waktu logger): 1,005 berarti host mencatat 1,005 s
    untuk 1,000 s nyata (jam host lambat, galat +5000 ppm pada rumus run_bench).
    """
    t0 = 3_600_000
    tepi = [] if tanpa_tepi else [(t0, 1), (t0 + 14_400_000, 0), (t0 + 36_000_000, 1), (t0 + 72_000_000, 0)]
    n = 556
    t_obs = round(1_000_000 * host_scale)
    t_sleep = t_sleep_us if t_sleep_us is not None else round(300_000 * host_scale)
    sampel = [(i * 144_000, sh, bus) for i in range(n)]
    sh_sum = sum(s[1] for s in sampel)
    bus_sum = sum(s[2] for s in sampel)
    if bus_rusak:                                       # satu nilai bus berubah, SUM tidak
        sampel[10] = (sampel[10][0], sampel[10][1], bus + 1)
    if cuplikan_hilang:
        sampel.pop()                                    # satu baris hilang di USB, SUM tetap n
    end = ["K1", 60, 1000, 1, t_obs, t_sleep, t_obs - t_sleep, 0, 2, 2, 0, 2, 0, 0, 2, 0, 0, 2,
           lat_cyc, lat_cyc, lat_cyc, 0, 48_000_000]
    assert len(end) == len(rb.END_COLS)
    h_lines = 4
    out = ["#RUN,BEGIN,1,4,60,72000000,4C364231", "h,1000000,>START,S60"]
    out += [f"s,{t},{a},{b}" for t, a, b in sampel]
    out += [f"e,{t},{lv}" for t, lv in tepi]
    out += ["h,76000000,>END," + ",".join(str(x) for x in end), "h,76100000,>L,9600000,9600000",
            "h,76200000,>DONE"]
    if not tanpa_sum:
        s = [1, 80_000_000, n, late, dt_max, 0, 0, 0, sh_sum, bus_sum, len(tepi), 0, 0, h_lines, 0]
        assert len(s) == len(rb.SUM_COLS)
        out.append("#SUM," + ",".join(str(x) for x in s))
    out.append(f"#RUN,END,1,{status}")
    return "\n".join(out) + "\n"


class TestParse(unittest.TestCase):
    def test_baris_rusak_diabaikan_dan_run_utuh_terbaca(self):
        teks = buat_log() + "s,1,2\ne,5\nbaris sampah\n"
        r = rb.parse(teks)
        self.assertEqual(len(r["s"]), 556)
        self.assertEqual(len(r["e"]), 4)
        self.assertEqual(r["status"], "OK")
        self.assertEqual(r["end"]["arrived"], 2)
        self.assertEqual(r["lat"], [9_600_000, 9_600_000])
        self.assertEqual(r["t_start"], 1_000_000)


class TestLayanan(unittest.TestCase):
    """Nilai harapan dihitung tangan dari angka tahap D (logbook 15 Nov)."""

    def en(self, dtg, lyn, ggl=0, bad=0, lost=0):
        return {"arrived": dtg, "served": lyn, "gagal": ggl, "rx_bad": bad, "rx_lost": lost}

    def test_barrage_software_jenuh(self):
        # K3: 604 datang, 91 dilayani, T = 60,464 s, sign 655,5 ms.
        # TL = 91/604 = 15,0662 %; lambda_eff = 604/60,464 = 9,98941; lambda x t = 6,54774; model = 15,2725 %.
        o = rb.layanan(self.en(604, 91), 60.464, 655.5)
        self.assertEqual(o["N_lwt"], 513)
        self.assertAlmostEqual(o["TL_pct"], 15.0662, places=3)
        self.assertAlmostEqual(o["R_lwt_pct"], 84.9338, places=3)
        self.assertAlmostEqual(o["Theta_max_sps"], 1 / 0.6555, places=6)       # 1,52555 sign/s
        self.assertAlmostEqual(o["TL_model_pct"], 15.2725, places=2)
        self.assertAlmostEqual(o["TL_selisih_pct"], 15.0662 - 15.2725, places=2)

    def test_normal_hardware_tidak_jenuh(self):
        # K2: 59/59, T = 60,301 s, sign 202,2 ms -> lambda x t = 0,198 < 1 -> model dibatasi 100 %.
        o = rb.layanan(self.en(59, 59), 60.301, 202.2)
        self.assertEqual(o["N_lwt"], 0)
        self.assertEqual(o["TL_pct"], 100.0)
        self.assertEqual(o["TL_model_pct"], 100.0)
        self.assertAlmostEqual(o["Theta_sps"], 59 / 60.301, places=9)
        self.assertAlmostEqual(o["Theta_max_sps"], 4.9456, places=3)           # 1/0,2022

    def test_frame_rusak_dihitung_terpisah_dari_terlewat(self):
        o = rb.layanan(self.en(100, 90, ggl=2, bad=3, lost=1), 10.0, 100.0)
        self.assertEqual(o["N_rusak"], 4)
        self.assertEqual(o["N_lwt"], 8)                 # 100 - 90 - 2
        self.assertAlmostEqual(o["TL_pct"], 90.0)

    def test_tanpa_permintaan_tidak_membagi_nol(self):
        o = rb.layanan(self.en(0, 0), 20.0, None)       # run IDLE
        self.assertNotIn("TL_pct", o)
        self.assertNotIn("TL_model_pct", o)
        self.assertEqual(o["Theta_sps"], 0.0)


class TestEnergiSesi(unittest.TestCase):
    def baris(self, arch, scen, p, i, **kw):
        r = {"arch": arch, "scen": scen, "P_avg_mW": p, "I_avg_mA": i, "T_obs_log_s": 60.0}
        r.update(kw)
        return r

    def test_nilai_dinamis_dan_per_layanan(self):
        # P_idle = (59,0 + 59,4) / 2 = 59,2 ; I_idle = (11,6 + 11,7) / 2 = 11,65
        # P_dyn = 82,2 - 59,2 = 23,0 ; E_dyn = 23,0 x 60 = 1380 mJ ; E_lyn = 1380 / 60 = 23,0 mJ
        # I_dyn = 16,3 - 11,65 = 4,65 ; Q_lyn = 4,65 x 60 / 60 = 4,65 mC
        rows = [self.baris("K1", "IDLE", 59.0, 11.6), self.baris("K1", "IDLE", 59.4, 11.7),
                self.baris("K1", "A", 82.2, 16.3, N_lyn=60)]
        rb.energi_sesi(rows)
        a = rows[2]
        self.assertAlmostEqual(a["P_idle_mW"], 59.2, places=9)
        self.assertAlmostEqual(a["P_dyn_mW"], 23.0, places=9)
        self.assertAlmostEqual(a["E_dyn_mJ"], 1380.0, places=6)
        self.assertAlmostEqual(a["E_lyn_mJ"], 23.0, places=9)
        self.assertAlmostEqual(a["I_dyn_mA"], 4.65, places=9)
        self.assertAlmostEqual(a["Q_lyn_mC"], 4.65, places=9)

    def test_idle_bermasalah_tidak_ikut_dasar(self):
        rows = [self.baris("K1", "IDLE", 59.0, 11.6), self.baris("K1", "IDLE", 99.0, 19.0, masalah="late"),
                self.baris("K1", "A", 82.0, 16.0, N_lyn=10)]
        rb.energi_sesi(rows)
        self.assertAlmostEqual(rows[2]["P_idle_mW"], 59.0, places=9)

    def test_arsitektur_tanpa_idle_dilewati(self):
        rows = [self.baris("K1", "IDLE", 59.0, 11.6), self.baris("K2", "A", 70.0, 13.0, N_lyn=10)]
        rb.energi_sesi(rows)
        self.assertNotIn("P_dyn_mW", rows[1])

    def test_dua_kali_panggil_hasilnya_sama(self):
        rows = [self.baris("K1", "IDLE", 59.0, 11.6), self.baris("K1", "A", 82.0, 16.0, N_lyn=10)]
        rb.energi_sesi(rows)
        pertama = dict(rows[1])
        rb.energi_sesi(rows)
        self.assertEqual(rows[1], pertama)

    def test_tanpa_layanan_tidak_ada_e_lyn(self):
        rows = [self.baris("K1", "IDLE", 59.0, 11.6), self.baris("K1", "B", 82.0, 16.0, N_lyn=0)]
        rb.energi_sesi(rows)
        self.assertIn("P_dyn_mW", rows[1])
        self.assertNotIn("E_lyn_mJ", rows[1])


class TestAnalyse(unittest.TestCase):
    def test_run_bersih(self):
        out, bad = rb.analyse(buat_log())
        self.assertEqual(bad, [])
        self.assertAlmostEqual(out["T_obs_log_s"], 1.0, places=9)
        self.assertAlmostEqual(out["S_cpu_log"], 30.0, places=9)        # LOW 0,3 s dari 1,0 s
        self.assertAlmostEqual(out["S_cpu_host"], 30.0, places=9)
        self.assertAlmostEqual(out["clk_ppm"], 0.0, places=6)
        # i = (402 - (-2)) x 2,5 uV / 0,1 ohm = 10,1 mA ; v = 4000 x 1,25 mV = 5,0 V ; P = 50,5 mW ; E = 50,5 mJ
        self.assertAlmostEqual(out["I_avg_mA"], 10.1, places=6)
        self.assertAlmostEqual(out["V_avg"], 5.0, places=9)
        self.assertAlmostEqual(out["P_avg_mW"], 50.5, places=6)
        self.assertAlmostEqual(out["E_mJ"], 50.5, places=6)
        self.assertAlmostEqual(out["I_model_mA"], 10.1, places=6)       # arus konstan: model = terukur
        # 9.600.000 siklus / 48 MHz = 200 ms
        self.assertAlmostEqual(out["lat_mean_ms"], 200.0, places=9)
        self.assertAlmostEqual(out["lat_mean_ms_nyata"], 200.0, places=9)
        self.assertEqual(out["TL_pct"], 100.0)
        self.assertAlmostEqual(out["Theta_sps"], 2.0, places=9)
        self.assertAlmostEqual(out["Theta_max_sps"], 5.0, places=9)

    def test_rshunt_lain_mengubah_arus_secara_terbalik(self):
        # R_shunt 0,08 ohm -> i = 1,01 mV / 0,08 = 12,625 mA (bukan 10,1)
        out, _ = rb.analyse(buat_log(), rshunt=0.08)
        self.assertAlmostEqual(out["I_avg_mA"], 12.625, places=6)

    def test_offset_dikurangkan(self):
        # offset 0 -> raw 402 dibaca 402 LSB -> 402 x 2,5 uV / 0,1 = 10,05 mA
        out, _ = rb.analyse(buat_log(), offset=0.0)
        self.assertAlmostEqual(out["I_avg_mA"], 10.05, places=6)

    def test_jam_host_lambat_dikoreksi(self):
        # Host mencatat 1,005 s untuk 1,000 s nyata: ppm = (1,005 / 1 - 1) x 1e6 = 5000.
        # Latensi nyata = nominal x (T_logger / T_host) = 200 / 1,005 = 199,00498 ms.
        out, bad = rb.analyse(buat_log(host_scale=1.005))
        self.assertEqual(bad, [])
        self.assertAlmostEqual(out["clk_ppm"], 5000.0, places=3)
        self.assertAlmostEqual(out["lat_mean_ms"], 200.0, places=9)
        self.assertAlmostEqual(out["lat_mean_ms_nyata"], 200 / 1.005, places=6)

    def test_cuplikan_hilang_di_usb_terdeteksi(self):
        _, bad = rb.analyse(buat_log(cuplikan_hilang=True))
        self.assertTrue(any("cuplikan diterima" in m for m in bad), bad)

    def test_checksum_tak_cocok_terdeteksi(self):
        _, bad = rb.analyse(buat_log(bus_rusak=True))
        self.assertTrue(any("checksum" in m for m in bad), bad)

    def test_status_bukan_ok_terdeteksi(self):
        _, bad = rb.analyse(buat_log(status="ERR_NOEND"))
        self.assertTrue(any("ERR_NOEND" in m for m in bad), bad)

    def test_tanpa_sum_dihentikan_dini(self):
        out, bad = rb.analyse(buat_log(tanpa_sum=True))
        self.assertIn("baris #SUM tidak ada", bad)

    def test_tanpa_tepi_pb1_terdeteksi(self):
        _, bad = rb.analyse(buat_log(tanpa_tepi=True))
        self.assertTrue(any("tepi PB1 tidak ada" in m for m in bad), bad)

    def test_scpu_host_beda_lebih_dari_setengah_poin_gugur(self):
        # t_sleep host 310.000 us dari 1.000.000 = 31,0 % vs logger 30,0 %: selisih 1,0 poin > 0,5
        _, bad = rb.analyse(buat_log(t_sleep_us=310_000))
        self.assertTrue(any("berbeda > 0,5 poin" in m for m in bad), bad)

    def test_late_ringan_diterima(self):
        # Aturan 15 Nov: late <= 10 dan selang terbesar < 5000 us (1 konversi terlewat ~4 ms) -> sah.
        out, bad = rb.analyse(buat_log(late=3, dt_max=288_000))        # 288.000 tik = 4000 us
        self.assertEqual(bad, [])
        self.assertEqual(out["late_ringan"], 3)

    def test_late_terlalu_banyak_gugur(self):
        _, bad = rb.analyse(buat_log(late=11, dt_max=288_000))
        self.assertTrue(any("late=11" in m for m in bad), bad)

    def test_late_selang_terlalu_besar_gugur(self):
        # 367.200 tik = 5100 us >= 5000
        _, bad = rb.analyse(buat_log(late=1, dt_max=367_200))
        self.assertTrue(any("late=1" in m for m in bad), bad)


if __name__ == "__main__":
    unittest.main()
