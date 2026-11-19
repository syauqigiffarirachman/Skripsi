# Proyeksi pemakaian baterai (ilustrasi)

Asumsi: baterai 1000 mAh x 3.7 V = 13.32 kJ; regulator diabaikan; arus diukur di input 5,2 V. Lantai papan (LED daya dkk.) 1,99 mA ikut pada kolom 'dengan lantai'; kolom 'tanpa lantai' hanya arus dinamis (batas atas, karena lantai chip sebenarnya 1-20 uA). Ini proyeksi dari data pengukuran, bukan umur baterai terukur.

| Kelompok | E per layanan (mJ) | Tanda tangan per baterai (ribu) | Arus rerata (mA) | Umur dengan lantai (jam) | Umur tanpa lantai (jam) | Tingkat layanan (%) |
|---|---|---|---|---|---|---|
| K1 Software, normal | 41.03 | 325 | 9.68 | 103 | 130 | 100.0 |
| K2 DS28E38, normal | 3.01 | 4426 | 2.55 | 392 | 1767 | 100.0 |
| K3 Software, barrage | 40.59 | 328 | 13.72 | 73 | 85 | 15.0 |
| K4 DS28E38, barrage | 3.00 | 4440 | 6.91 | 145 | 203 | 85.5 |

Catatan: pada barrage kedua arsitektur melayani jumlah permintaan berbeda (K3 15 %, K4 85,5 %), sehingga umur per baterai K3 vs K4 tidak setara pekerjaan; bandingkan lewat kolom 'tanda tangan per baterai'.
