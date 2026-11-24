# Gambar Bab 5 untuk naskah (kalibrasi 21 Nov 2026)

Dibuat 2026-11-21 14:36 oleh `tools/ekspor_gambar_naskah.py`. ringkasan.md: rshunt=0.0821, offset=-2.83, u_rel=0.015; kalibrasi_param: 0.0821, -2.83, 0.015.

Penomoran = naskah v4 (Gambar 5.x); sumber memakai penomoran lama 4.x. Ganti gambar di Word dengan berkas pada kolom 2.
Status: BERUBAH = nilai atau sumbu berbeda dari v4 (wajib diganti); tetap = isi sama (boleh dibiarkan, berkas disertakan agar seragam).

| Gambar | Berkas | Sumber | Status | Perubahan |
|---|---|---|---|---|
| 5.1 | `Gambar_5.1_frame_hilang_K4.png` | `gambar_bab4/gambar_4_10_frame_hilang_K4.png` | tetap | jumlah frame; tidak bergantung kalibrasi |
| 5.2 | `Gambar_5.2_bland_altman_durasi_tidur.png` | `gambar_bab4/gambar_4_11_bland_altman_scpu.png` | tetap | poin persentase; tidak bergantung kalibrasi |
| 5.3 | `Gambar_5.3_arus_diam_antar_blok.png` | `gambar_bab4/gambar_4_9_arus_diam_antar_blok.png` | BERUBAH | arus diam 1,99 -> ~2,45 mA; sumbu mengikuti data |
| 5.4 | `Gambar_5.4_jejak_arus_3_detik.png` | `gambar_bab4/gambar_4_1_jejak_arus.png` | BERUBAH | sumbu arus 0-26 mA; arus diam 2,45 mA; puncak K2/K4 23,8 mA utuh |
| 5.5 | `Gambar_5.5_jejak_arus_rinci.png` | `gambar_bab4/gambar_4_1b_jejak_arus_rinci.png` | BERUBAH | sumbu arus 0-26 mA |
| 5.6 | `Gambar_5.6_latensi_tanda_tangan.png` | `gambar_bab4/gambar_4_2_latensi.png` | tetap | ms; tidak bergantung kalibrasi |
| 5.7 | `Gambar_5.7_durasi_tidur_cpu.png` | `gambar_bab4/gambar_4_3_durasi_tidur_cpu.png` | tetap | % waktu; tidak bergantung kalibrasi |
| 5.8 | `Gambar_5.8_daya_dinamis.png` | `gambar_bab4/gambar_4_4_daya_dinamis.png` | BERUBAH | mW x1,218; batang galat Ugab 1,5 %; teks +-1,5 % |
| 5.9 | `Gambar_5.9_energi_per_permintaan.png` | `gambar_bab4/gambar_4_5_energi_per_permintaan.png` | BERUBAH | mJ x1,218; batang galat Ugab 1,5 %; teks +-1,5 % |
| 5.10 | `Gambar_5.10_interaksi_arsitektur_skenario.png` | `gambar_bab4/gambar_4_6_interaksi.png` | BERUBAH | panel (b) daya dinamis mW; panel (a) durasi tidur sama |
| 5.11 | `Gambar_5.11_tingkat_layanan_vs_model.png` | `gambar_bab4/gambar_4_7_tingkat_layanan_vs_model.png` | tetap | % permintaan; tidak bergantung kalibrasi |
| 5.12 | `Gambar_5.12_throughput_vs_batas.png` | `gambar_bab4/gambar_4_8_throughput_vs_batas.png` | tetap | sign/s; tidak bergantung kalibrasi |
| baru | `Gambar_baru_dekomposisi_energi_opsional.png` | `gambar_bab4/gambar_4_12_dekomposisi_energi.png` | BARU (opsional) | memperkuat Tabel 5.10; v4 hanya memuat tabel |

Diagram Bab 2-3 (Gambar 2.1, 3.1-3.5) digambar terpisah dan TIDAK dibuat skrip ini; lihat laporan/laporan_perubahan_skripsi_v4_kalibrasi_2026-11-21.md bagian H.
