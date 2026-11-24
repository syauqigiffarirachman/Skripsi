# Bangku ukur ECDSA P-256: software vs DS28E38

Penelitian membandingkan tanda tangan ECDSA P-256 yang dihitung software di mikrokontroler dengan yang dilakukan chip
DS28E38, diukur pada arus, daya, energi, latensi, dan layanan. Kamus ini membakukan istilah yang dipakai kode, data, dan naskah.

## Arsitektur yang dibandingkan

**K1**:
Host menandatangani dengan software (Mbed TLS), beban Normal.
_Hindari_: baseline software, SW normal

**K2**:
Host menandatangani lewat chip DS28E38, beban Normal.
_Hindari_: HW normal, offloading normal

**K3**:
Seperti K1 tetapi beban Barrage.

**K4**:
Seperti K2 tetapi beban Barrage.

**K1S / K2S**:
K1 dan K2 dengan CPU tidur di Stop mode di antara permintaan. Inilah arsitektur pada **data akhir**.
_Hindari_: K1 Stop, varian deep sleep (sebagai nama arsitektur)

**Tahap D**:
Pengambilan data awal, CPU tidur Sleep (WFI) di antara permintaan. Bukan data akhir; disimpan sebagai pembanding pengembangan.

## Bangku ukur

**Host**:
Board yang menandatangani permintaan; firmware diganti per arsitektur.
_Hindari_: DUT, target

**Logger**:
Board pembanding independen yang merekam arus host, tepi pin status, dan baris kendali, lalu mengirim CSV ke PC.
_Hindari_: pencatat (sebagai nama board), monitor

**Attacker**:
Board pembangkit permintaan ke host.
_Hindari_: penyerang (di kode dan data), generator beban

**Frame**:
Satu permintaan dari attacker ke host.
_Hindari_: paket, pesan

**Pin status**:
Sinyal host yang bernilai 1 saat CPU aktif dan 0 saat akan tidur; dasar waktu aktif yang dilihat logger.

## Skenario dan data

**Skenario A (Normal)**:
Satu permintaan per detik.

**Skenario B (Barrage)**:
Sepuluh permintaan per detik, melebihi kapasitas software.

**IDLE**:
Jendela tanpa permintaan; memberi arus dan daya diam papan.

**Run**:
Satu jendela pengamatan 60 detik pada satu skenario, tersimpan sebagai satu CSV mentah.

**Sesi / Blok**:
Sesi adalah satu folder run pada satu arsitektur; blok adalah posisi sesi dalam urutan ABBA (K2S, K1S, K1S, K2S) yang menyeimbangkan pengaruh waktu.

**Jendela pengamatan**:
Selang T_obs yang diukur; di awal dan akhirnya host menandai pin status.

**Run sah**:
Run yang lolos pemeriksaan keutuhan (cuplikan tidak terlambat, kecocokan S_cpu logger dan host, tanpa frame rusak/hilang di luar toleransi).
_Hindari_: run valid, run bagus

## Metrik

**T_obs**:
Lama jendela pengamatan.

**S_cpu**:
Persentase T_obs ketika CPU tidur.
_Hindari_: duty sleep

**N_dtg / N_lyn / N_ggl / N_lwt**:
Permintaan diterima / dilayani (tanda tangan selesai) / gagal / terlewat karena host sedang sibuk (tidak diantrekan).

**Tingkat layanan (TL)**:
N_lyn dibagi N_dtg, dalam persen.
_Hindari_: success rate

**Throughput (Theta)**:
Tanda tangan dilayani per detik.

**Latensi sign**:
Waktu satu tanda tangan dalam milidetik nyata (dikoreksi terhadap jam kristal logger).

**P_idle / P_dyn**:
Daya diam papan pada arsitektur dan blok yang sama; P_dyn = daya rata-rata dikurangi P_idle. Arus (I_idle, I_dyn) sejajar.

**E_lyn**:
Energi dinamis per permintaan dilayani = P_dyn x T_obs / N_lyn.
_Hindari_: energi per sign (nama kolom lama E_per_sign)

**Lantai papan**:
Arus diam terendah papan uji (sekitar 2,45 mA terkalibrasi), terutama dari LED daya; bukan sifat chip.

## Kalibrasi

**R_eff**:
Hambatan shunt efektif INA226 hasil kalibrasi (0,0821 Ohm), bukan nilai 0,100 Ohm yang tertulis di modul.
_Hindari_: R_shunt nominal

**Offset**:
Pembacaan shunt mentah saat tanpa beban (-2,83 satuan 2,5 uV).

**u_kal**:
Ketidakpastian relatif kalibrasi arus (1,5 %), komponen sistematik dalam U_gab.

## Pemisahan kode

**Final**:
Kode yang menghasilkan data akhir 18 Nov 2026: env `host_k1_stop`, `host_k2_stop`, `logger`, `attacker` dan skrip PC dari akuisisi sampai analisis.

**Pendukung**:
Kode final yang sama dengan satu flag penambah (`*_auto`, `DIAG_*`, comb, dbg). Bukti tambahan, bukan data utama.

**Dev**:
Kode pengembangan yang tidak ada pada data akhir (tangga menuju Stop, tahap D, eksperimen).
_Hindari_: legacy, lama
