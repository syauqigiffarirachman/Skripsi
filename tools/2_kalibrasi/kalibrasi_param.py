"""Parameter kalibrasi INA226 FINAL (21 November 2026): SATU sumber nilai untuk default baris perintah.

Dasar dan keterbatasan: data/kalibrasi/kalibrasi_final_2026-11-21.md
  - R_eff   = 0,0821 Ohm : rerata 6 selisih dI = V_bus / R_beban (resistor 1 kOhm x3, 2,2 kOhm x3);
                           shunt modul berkode R100 ternyata bukan 0,100 Ohm
  - offset  = -2,83 satuan (2,5 uV/satuan): rerata awal -2,820 dan akhir -2,833 (beban nol)
  - u_rel   = 0,015 : ketidakpastian relatif kalibrasi (metode selisih 1,2 %; pembanding multimeter menyimpang 1,4 %)
Kalibrasi diverifikasi pada 12-21 mA (kelompok terjaga); linearitas sampai +-2 mA (Stop) diasumsikan.

Dipakai SEBAGAI DEFAULT argumen --rshunt / --offset / --u-rel di skrip yang menghasilkan angka mutlak naskah
(run_bench, analisis_d, dekomposisi_energi, gambar_bab4_stop, hitung_ulang_satu_run, demo_sidang, uji_galat_stop, k1s_run).
Default FUNGSI (mis. run_bench.analyse(text, rshunt=0.100, offset=-2.0)) sengaja tetap nominal: uji di tests/ menghitung
nilai harapan dengan tangan pada 0,100 Ohm. Untuk mereproduksi hasil lama: --rshunt 0.100 --offset -2.0 --u-rel 0.05.
Skrip uji tahap awal (s0_stop, s1_rtc, s2_rx, a3_clock) tetap bernilai nominal; konversi angkanya: I_baru = 1,218 x I_lama + 0,025 mA.
"""
SHUNT_LSB_V = 2.5e-6       # tegangan shunt INA226 per satuan mentah (2,5 uV)
RSHUNT_NOMINAL = 0.100     # Ohm; nilai tertulis pada modul (dipakai analisis sebelum 21 Nov 2026)
OFFSET_NOMINAL = -2.0      # satuan mentah (2,5 uV)
RSHUNT_EFEKTIF = 0.0821    # Ohm; hasil kalibrasi 21 Nov 2026
OFFSET_LSB = -2.83         # satuan mentah (2,5 uV)
U_REL_KAL = 0.015          # pecahan (bukan persen): 0,015 = 1,5 %
