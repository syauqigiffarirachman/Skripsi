# Proyeksi pemakaian baterai (ilustrasi)

Asumsi: baterai 1000 mAh x 3.7 V = 13.32 kJ; regulator diabaikan; arus diukur di input 5,2 V. Lantai papan (LED daya dkk.) ~2,45 mA terkalibrasi (1,99 mA pada shunt nominal 0,100 Ohm) ikut pada kolom 'dengan lantai'; kolom 'tanpa lantai' hanya arus dinamis (batas atas, karena lantai chip sebenarnya 1-20 uA). Ini proyeksi dari data pengukuran, bukan umur baterai terukur.

| Kelompok | E per layanan (mJ) | Tanda tangan per baterai (ribu) | Arus rerata (mA) | Umur dengan lantai (jam) | Umur tanpa lantai (jam) | Tingkat layanan (%) |
|---|---|---|---|---|---|---|
| K1 Software, normal | 49.98 | 267 | 11.82 | 85 | 107 | 100.0 |
| K2 DS28E38, normal | 3.67 | 3634 | 3.14 | 319 | 1451 | 100.0 |
| K3 Software, barrage | 49.44 | 269 | 16.73 | 60 | 70 | 15.0 |
| K4 DS28E38, barrage | 3.65 | 3645 | 8.45 | 118 | 167 | 85.5 |

Catatan: pada barrage kedua arsitektur melayani jumlah permintaan berbeda (K3 15 %, K4 85,5 %), sehingga umur per baterai K3 vs K4 tidak setara pekerjaan; bandingkan lewat kolom 'tanda tangan per baterai'.
