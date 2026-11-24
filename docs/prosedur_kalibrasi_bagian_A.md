# Prosedur Bagian A: kalibrasi shunt, offset, pull-up, osiloskop

Disusun 18 November 2026. Alat bantu: `tools/2_kalibrasi/kalibrasi_shunt.py` (merekam lewat logger dan menghitung). Hasil disimpan di
`data/kalibrasi/`. Perkiraan waktu: 90–120 menit.

## Mengapa ini perlu

- **Shunt efektif.** Catatan 15 Nov menduga shunt modul "R100" sebenarnya 0,080–0,083 Ω, tetapi acuannya resistor beban yang diukur
  dengan ohmmeter murah, dan multimeter yang sama membaca tegangan sekitar 5 % rendah. Jadi angka itu belum bisa dipakai sebagai
  koreksi. Bila benar, nilai mutlak arus, daya, dan energi di Bab 4 berubah sampai ±20 %, bukan ±5 %.
- **Offset.** Analisis memakai offset −2,0 satuan (2,5 µV/satuan). Pengukuran 15 Nov: −2 satuan (A2b), lalu −3,4 satuan malam harinya, dan
  pembacaan 18 Nov saat host mati sekitar −3,2 satuan. Selisih ±1,2 satuan = ±0,03 mA, sekitar 1,5 % dari lantai 1,99 mA. Tidak
  memengaruhi metrik "dinamis" (offset yang sama terhapus oleh pengurangan IDLE), tetapi memengaruhi nilai mutlak.

## Alat yang dibutuhkan

| Alat | Catatan |
|---|---|
| Multimeter dengan rentang mA DC | Catat merek, tipe, rentang yang dipakai, dan spesifikasi akurasi (misal ±(0,8 % + 3 digit)). Idealnya multimeter lain yang sudah terverifikasi, bukan yang membaca tegangan 5 % rendah |
| Resistor 1 % (atau lebih baik): 2,2 kΩ dan 1 kΩ | Ukur nilainya dengan multimeter terbaik yang ada; catat toleransi |
| Kabel jumper pendek, penjepit buaya | Multimeter dipasang SERI pada jalur 5 V (USB sisi komputer, sebelum IN+ INA226) |

Catatan multimeter seri: rentang mA menambah jatuh tegangan kecil; catat tegangan bus yang terbaca skrip. Jangan memakai rentang µA.

## Urutan kerja

Kondisi awal: host berisi K2S (sedang terjaga di luar jendela, arus ±12,7 mA, keadaan bersih). Logger di COM18.
**Jangan membuka OpenOCD sebelum titik terjaga selesai.**

### A2. Offset nol (kerjakan dulu, dua kali: awal dan akhir sesi)

1. Lepas **IN− dari host** (kabel dari terminal IN− INA226 ke pin 5V host). Biarkan VBUS/IN+ INA226 tetap terpasang dan logger menyala.
2. Jalankan: `python tools/2_kalibrasi/kalibrasi_shunt.py offset --detik 60 --catatan awal`
3. Sambungkan kembali IN− ke host. Ulangi di akhir seluruh sesi dengan `--catatan akhir`.
4. Catat nilainya. Bila selisih awal dan akhir > 0,5 satuan, ulangi setelah 10 menit penghangatan.

### A1. Kalibrasi shunt: titik arus tunak (terjaga)

Setiap titik: pasang multimeter seri, tunggu stabil 15 detik, jalankan perintah, **sambil membaca multimeter** (catat 3 pembacaan,
ketik rerata sebagai `--iref`).

| Titik | Keadaan | Perintah |
|---|---|---|
| T1 | host terjaga di luar jendela (K2S), tanpa beban tambahan (±12,7 mA) | `python tools/2_kalibrasi/kalibrasi_shunt.py titik --nama terjaga --iref <mA> --detik 30` |
| T2 | T1 + resistor 1 kΩ dari pin 5V host ke GND (±17,9 mA) | `... titik --nama terjaga_1k --iref <mA> --vref <V>` |
| T3 | T1 + resistor 2,2 kΩ (±15,0 mA) | `... titik --nama terjaga_2k2 --iref <mA> --vref <V>` |

`--vref` = tegangan di pin 5V host (multimeter DCV, paralel) bila Anda sempat mengukurnya. Pemeriksaan silang: arus resistor = V/R harus
sama dengan selisih arus T2 dan T1 (dalam ±1 %).

### A1b. Titik arus rendah (Stop), butuh pemrograman ulang

1. Flash `host_s0_stop` (Stop terus-menerus): `python tools/1_akuisisi/flash_stop.py .pio/build/host_s0_stop/firmware.elf`. Bila gagal menangkap host, pakai
   `tools/dev/tahap_stop/pulihkan_host.py` (BOOT0) seperti biasa.
2. **POR penuh** (USB-C, ST-Link, kabel sinyal dan GND dicabut 10 detik; GND dipasang dulu). Tunggu 10 detik: host menjalankan jendela 5 detik lalu Stop selamanya.
3. Titik:

| Titik | Keadaan | Perintah |
|---|---|---|
| T4 | host Stop murni (±1,99 mA) | `... titik --nama stop --iref <mA>` |
| T5 | T4 + resistor 2,2 kΩ (±4,3 mA) | `... titik --nama stop_2k2 --iref <mA> --vref <V>` |
| T6 | T4 + resistor 1 kΩ (±7,2 mA) | `... titik --nama stop_1k --iref <mA> --vref <V>` |

Pada arus ±2 mA, resolusi multimeter menentukan: tulis resolusinya di catatan. Bila multimeter tidak cukup baik di 2 mA, andalkan T5 dan T6
dengan pemeriksaan V/R.

### Menghitung

`python tools/2_kalibrasi/kalibrasi_shunt.py hitung --u-ref <persen akurasi multimeter pada titik itu>`

Keluaran: R_shunt efektif, faktor koreksi arus, residu per titik, intersep (cek offset), u_kal sebenarnya, dan perintah siap pakai untuk
menjalankan ulang `tools/3_analisis/analisis_d.py` dan `tools/3_analisis/gambar_bab4_stop.py`. Kirim saya isi `data/kalibrasi/*.csv`, nilai resistor, spesifikasi
multimeter, dan catatan sesi; saya jalankan ulang analisis dan gambar, lalu perbarui Bab 3 dan panduan grafik.

Kriteria penerimaan: residu relatif tiap titik < 2 %; intersep regresi bebas < 0,05 mA; hasil regresi stabil saat satu titik dibuang.

## A3. Pull-up 1-Wire

1. Matikan host (cabut USB-C), cabut papan DS9121 dari soket.
2. Ukur resistansi antara **kaki PA15** (jalur 1-Wire pada soket) dan **3V3** dengan ohmmeter. Catat nilai (harapan 4,7 kΩ ±5 %).
3. Ukur juga dengan papan DS9121 terpasang (bila papan itu menambah pull-up atau jalur paralel, nilainya turun).
4. Catat posisi jumper VCC/CEXT pada papan DS9121 (foto). Tulis ke logbook.

Sertakan di Bab 3 Tabel 3.1/3.3: nilai terukur, bukan dugaan.

## A4. Osiloskop (hanya bila osiloskop benar dipakai)

Bila tidak dipakai, kabari saya; baris osiloskop di Tabel 3.1 dihapus.

| Tangkapan | Saluran | Pemicu | Tujuan |
|---|---|---|---|
| 1 | PB1 (pin status) + PA15 (1-Wire) | tepi naik PB1, K2S satu permintaan, 200 ms/div | bukti urutan: aktif, kirim perintah, tidur, baca hasil |
| 2 | PA15 zoom satu bit overdrive, 2 µs/div | tepi turun PA15 | bukti pewaktuan overdrive (A = 1 µs, titik baca 2,5 µs) |
| 3 | PA3 (frame) + PB1 | tepi turun PA3 saat host tidur | bukti bangun dari Stop dan pola byte 0xA5 |
| 4 | rail 3V3 host | tepi naik PB1 | bukti tidak ada brown-out saat bangun dari Stop |

Simpan CSV atau gambar di `data/kalibrasi/osiloskop/` dengan nama `tangkapan_<n>_<keterangan>.*`, catat skala vertikal/horizontal dan
probe (x1/x10, jangan menyentuh PC14/PC15).

## Yang saya perlukan kembali

1. `data/kalibrasi/offset.csv` dan `titik.csv` (otomatis).
2. Spesifikasi dan merek multimeter; nilai terukur tiap resistor; tegangan V di resistor bila diukur.
3. Hasil pull-up (A3) dan keputusan osiloskop (A4).
4. Catatan bila ada titik yang terganggu (kabel longgar, host reset).
