"""Uji skenario C (kapasitas, rho = 1; 27 Nov 2026) dengan data SINTETIS yang nilainya dihitung tangan.

Jalankan: .venv-analisis/Scripts/python -B -m unittest discover -s tests -v
Nilai harapan dihitung dengan kalkulator lebih dulu (aturan tests/test_metrik.py), bukan disalin dari keluaran fungsi.
"""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import _jalur  # noqa: E402,F401
import periode_kapasitas as pk  # noqa: E402


def baris(arch, scen, masalah="", **kw):
    r = {"arch": arch, "scen": scen, "masalah": masalah}
    r.update(kw)
    return r


class TestPeriode(unittest.TestCase):
    def test_periode_dasar(self):
        rows = [baris("K2S", "A", lat_mean_ms_nyata=116.0, lat_n=60, N_dtg=60, T_obs_log_s=60.0),
                baris("K2S", "A", lat_mean_ms_nyata=116.6, lat_n=40, N_dtg=59, T_obs_log_s=60.0),
                baris("K2S", "B", N_dtg=600, T_obs_log_s=60.0, lat_mean_ms_nyata=116.5, lat_n=514),
                baris("K2S", "B", N_dtg=602, T_obs_log_s=60.2, lat_mean_ms_nyata=116.5, lat_n=514)]
        h = pk.hitung_periode(rows)["K2S"]
        self.assertAlmostEqual(h["t_sign_ms"], 116.24, places=9)      # (116*60 + 116.6*40) / 100
        self.assertEqual(h["n_sign"], 100)
        self.assertAlmostEqual(h["lambda_B"], 10.0, places=9)        # 1202 / 120.2
        self.assertAlmostEqual(h["k"], 1.0, places=9)
        self.assertEqual(h["periode_kirim_us"], 116240)
        self.assertEqual(h["periode_nyata_us"], 116240)

    def test_jam_attacker_lambat(self):
        rows = [baris("K1S", "A", lat_mean_ms_nyata=654.9, lat_n=59, N_dtg=59, T_obs_log_s=60.0),
                baris("K1S", "B", N_dtg=594, T_obs_log_s=60.0, lat_mean_ms_nyata=655.0, lat_n=92)]
        h = pk.hitung_periode(rows)["K1S"]
        self.assertAlmostEqual(h["lambda_B"], 9.9, places=9)
        self.assertAlmostEqual(h["k"], 1.0101010101, places=9)       # 10 / 9.9
        self.assertEqual(h["periode_kirim_us"], 648351)               # 654900 * 9.9 / 10
        self.assertEqual(h["periode_nyata_us"], 654900)

    def test_run_bermasalah_diabaikan(self):
        rows = [baris("K2S", "A", lat_mean_ms_nyata=116.0, lat_n=60, N_dtg=60, T_obs_log_s=60.0),
                baris("K2S", "A", masalah="host: hilang=3", lat_mean_ms_nyata=200.0, lat_n=60, N_dtg=60,
                      T_obs_log_s=60.0),
                baris("K2S", "B", N_dtg=600, T_obs_log_s=60.0),
                baris("K2S", "B", masalah="logger late=40", N_dtg=500, T_obs_log_s=60.0)]
        h = pk.hitung_periode(rows)["K2S"]
        self.assertAlmostEqual(h["t_sign_ms"], 116.0, places=9)
        self.assertEqual(h["n_sign"], 60)
        self.assertAlmostEqual(h["lambda_B"], 10.0, places=9)

    def test_tanpa_B_gagal(self):
        rows = [baris("K2S", "A", lat_mean_ms_nyata=116.0, lat_n=60, N_dtg=60, T_obs_log_s=60.0)]
        with self.assertRaises(ValueError):
            pk.hitung_periode(rows)

    def test_tanpa_A_gagal(self):
        rows = [baris("K2S", "B", N_dtg=600, T_obs_log_s=60.0)]
        with self.assertRaises(ValueError):
            pk.hitung_periode(rows)


try:
    import run_bench as rb  # noqa: E402
except ModuleNotFoundError as e:                       # mis. pyserial belum terpasang
    rb = None
    _alasan = str(e)

PERIODE = {"K2S": {"periode_kirim_us": 116240, "periode_nyata_us": 116240, "t_sign_ms": 116.24}}


@unittest.skipIf(rb is None, "run_bench tak bisa diimpor")
class TestRunBenchC(unittest.TestCase):
    def test_perintah_attacker(self):
        self.assertEqual(rb.perintah_attacker("A", "K2S", PERIODE), "a")
        self.assertEqual(rb.perintah_attacker("B", "K2S", PERIODE), "b")
        self.assertIsNone(rb.perintah_attacker("IDLE", "K2S", PERIODE))
        self.assertEqual(rb.perintah_attacker("C", "K2S", PERIODE), "p116240\n")

    def test_perintah_attacker_C_tanpa_periode(self):
        with self.assertRaises(SystemExit):
            rb.perintah_attacker("C", "K1S", PERIODE)
        with self.assertRaises(SystemExit):
            rb.perintah_attacker("C", "K2S", None)

    def test_idle_luar(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "runs.csv"
            p.write_text("arch,scen,masalah,P_avg_mW,I_avg_mA\n"
                         "K2,IDLE,,12.0,2.4\n"
                         "K2,IDLE,,13.0,2.5\n"
                         "K2,IDLE,host rusak,99.0,9.9\n"
                         "K2,B,,43.0,8.4\n"
                         "K1,IDLE,,12.8,2.45\n", encoding="utf-8")
            h = rb.idle_luar(p)
        self.assertEqual(set(h), {"K1S", "K2S"})
        self.assertAlmostEqual(h["K2S"][0], 12.5, places=9)
        self.assertAlmostEqual(h["K2S"][1], 2.45, places=9)
        self.assertAlmostEqual(h["K1S"][0], 12.8, places=9)

    def test_energi_sesi_idle_luar(self):
        rows = [{"arch": "K2S", "scen": "C", "masalah": "", "P_avg_mW": 42.5, "I_avg_mA": 8.3,
                 "T_obs_log_s": 60.0, "N_lyn": 516}]
        rb.energi_sesi(rows, idle_luar={"K2S": (12.5, 2.45)})
        r = rows[0]
        self.assertAlmostEqual(r["P_dyn_mW"], 30.0, places=9)
        self.assertAlmostEqual(r["E_lyn_mJ"], 3.488372093, places=8)      # 30 * 60 / 516
        self.assertAlmostEqual(r["I_dyn_mA"], 5.85, places=9)
        self.assertEqual(r["P_idle_sumber"], "stop_final")

    def test_energi_sesi_idle_sesi_didahulukan(self):
        rows = [{"arch": "K2S", "scen": "IDLE", "masalah": "", "P_avg_mW": 12.0, "I_avg_mA": 2.4,
                 "T_obs_log_s": 60.0},
                {"arch": "K2S", "scen": "A", "masalah": "", "P_avg_mW": 15.0, "I_avg_mA": 3.0,
                 "T_obs_log_s": 60.0, "N_lyn": 60}]
        rb.energi_sesi(rows, idle_luar={"K2S": (99.0, 9.9)})
        self.assertAlmostEqual(rows[1]["P_dyn_mW"], 3.0, places=9)
        self.assertEqual(rows[1]["P_idle_sumber"], "sesi")

    def test_rho(self):
        r = {"arch": "K2S", "scen": "C", "lambda_eff_rps": 8.6}
        rb.tambah_kapasitas(r, PERIODE)
        self.assertAlmostEqual(r["rho_nyata"], 0.999664, places=9)          # 8.6 * 0.11624
        self.assertEqual(r["periode_us"], 116240)

    def test_rho_bukan_C_tidak_disentuh(self):
        r = {"arch": "K2S", "scen": "B", "lambda_eff_rps": 10.0}
        rb.tambah_kapasitas(r, PERIODE)
        self.assertNotIn("rho_nyata", r)

    def test_kelompok_C(self):
        self.assertEqual(rb.KELOMPOK[("K1S", "C")], "K5S")
        self.assertEqual(rb.KELOMPOK[("K2S", "C")], "K6S")


def log_hilang(lost, arrived=515, rusak=0):
    """Run sintetis test_metrik.buat_log dengan baris >END diganti: arsitektur K2S, `arrived` tiba, `lost` hilang."""
    from test_metrik import buat_log
    baris = []
    for ln in buat_log().splitlines():
        if ">END," in ln:
            kepala, isi = ln.split(">END,")
            v = isi.split(",")
            v[0], v[8], v[14], v[15], v[16] = "K2S", str(arrived), str(arrived - lost - rusak), str(rusak), str(lost)
            ln = kepala + ">END," + ",".join(v)
        baris.append(ln)
    return "\n".join(baris) + "\n"


def masalah_host(bad):
    return [b for b in bad if b.startswith("host:")]


@unittest.skipIf(rb is None, "run_bench tak bisa diimpor")
class TestToleransiHilangC(unittest.TestCase):
    """Keputusan peneliti 27 Nov: skenario C boleh kehilangan frame < 1 % dari yang tiba (frame rusak tetap 0);
    A/B tetap < 0,25 % (keputusan 18 Nov)."""

    def test_C_hilang_di_bawah_1_persen_sah(self):
        out, bad = rb.analyse(log_hilang(4), scen="C")           # 4 / 515 = 0,78 %
        self.assertEqual(masalah_host(bad), [])
        self.assertEqual(out["rx_lost_toleransi"], 4)

    def test_C_hilang_1_persen_atau_lebih_gugur(self):
        _, bad = rb.analyse(log_hilang(6), scen="C")             # 6 / 515 = 1,17 %
        self.assertEqual(len(masalah_host(bad)), 1)

    def test_C_frame_rusak_tetap_gugur(self):
        _, bad = rb.analyse(log_hilang(1, rusak=1), scen="C")
        self.assertEqual(len(masalah_host(bad)), 1)

    def test_B_tetap_025_persen(self):
        _, bad = rb.analyse(log_hilang(4), scen="B")             # 0,78 % > 0,25 %
        self.assertEqual(len(masalah_host(bad)), 1)
        _, bad = rb.analyse(log_hilang(1), scen="B")             # 0,19 % < 0,25 %
        self.assertEqual(masalah_host(bad), [])

    def test_tanpa_scen_tetap_025_persen(self):
        _, bad = rb.analyse(log_hilang(4))
        self.assertEqual(len(masalah_host(bad)), 1)


try:
    import numpy as np  # noqa: E402
    import pandas as pd  # noqa: E402
    import scipy  # noqa: E402,F401
    import statsmodels  # noqa: E402,F401
except ModuleNotFoundError:                            # pandas/scipy hanya di .venv-analisis
    ak = None
else:
    import analisis_kapasitas as ak  # noqa: E402   (modul hilang = galat, bukan dilewati)


@unittest.skipIf(ak is None, "pandas/scipy/statsmodels tidak tersedia")
class TestAnalisisKapasitas(unittest.TestCase):
    def test_idle_dari_df(self):
        df = pd.DataFrame({"arch": ["K2", "K2", "K2", "K1"], "scen": ["IDLE", "IDLE", "B", "IDLE"],
                           "sah": [True, True, True, False], "P_avg_mW": [12.0, 13.0, 40.0, 99.0],
                           "I_avg_mA": [2.4, 2.5, 8.0, 9.9]})
        h = ak.idle_dari_df(df)
        self.assertEqual(set(h), {"K2S"})                   # IDLE K1 tidak sah -> tidak ada
        self.assertAlmostEqual(h["K2S"][0], 12.5, places=9)
        self.assertAlmostEqual(h["K2S"][1], 2.45, places=9)

    def test_banding_selisih_dan_holm(self):
        # Tiga pasangan dengan data identik -> p_welch sama; Holm: p_holm = min(1, 3 * p) untuk ketiganya.
        a, b = [1.0, 2.0, 3.0, 2.0], [4.0, 5.0, 6.0, 5.0]
        rows = []
        for k in ("X1", "X2", "X3"):
            rows += [{"kelompok": k, "m": v} for v in a]
        for k in ("Y1", "Y2", "Y3"):
            rows += [{"kelompok": k, "m": v} for v in b]
        t = ak.banding(pd.DataFrame(rows), [("X1", "Y1"), ("X2", "Y2"), ("X3", "Y3")], ["m"], [],
                       np.random.default_rng(1), n_boot=200)
        self.assertEqual(len(t), 3)
        self.assertTrue(np.allclose(t["rerata_1"], 2.0))
        self.assertTrue(np.allclose(t["rerata_2"], 5.0))
        self.assertTrue(np.allclose(t["selisih_2_min_1"], 3.0))
        p = t["p_welch"].iloc[0]
        self.assertTrue(np.allclose(t["p_holm"], min(1.0, 3 * p)))

    def test_banding_tanpa_data_tidak_crash(self):
        rows = [{"kelompok": "X", "m": v} for v in (1.0, 2.0, 3.0)]
        t = ak.banding(pd.DataFrame(rows), [("X", "Y")], ["m"], [], np.random.default_rng(1), n_boot=50)
        self.assertEqual(len(t), 0)
        self.assertIn("p_holm", t.columns)

    def test_banding_sekunder_tanpa_holm(self):
        rows = [{"kelompok": "X", "m": v, "s": v} for v in (1.0, 2.0, 3.0)]
        rows += [{"kelompok": "Y", "m": v, "s": v} for v in (4.0, 5.0, 7.0)]
        t = ak.banding(pd.DataFrame(rows), [("X", "Y")], ["m"], ["s"], np.random.default_rng(1), n_boot=200)
        self.assertTrue(np.isnan(t.loc[t["metrik"] == "s", "p_holm"].iloc[0]))
        self.assertFalse(np.isnan(t.loc[t["metrik"] == "m", "p_holm"].iloc[0]))


if __name__ == "__main__":
    unittest.main()
