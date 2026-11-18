# 06. Latihan dan Kuis (jawab dulu, baru buka jawaban)

Cara pakai: tutup berkas lain, tulis jawabanmu, baru cek di bagian jawaban. Soal bertanda (S) = pertanyaan yang
mungkin muncul di sidang.

## Soal

### Kriptografi
1. Tulis alur tanda tangan ECDSA dan jelaskan mengapa nonce `k` tidak boleh bocor atau dipakai ulang.
2. (S) Mengapa K1 memakai nonce hedged, bukan RFC 6979 murni dan bukan acak murni?
3. Mengapa `mbedtls_platform_zeroize` dipakai alih-alih `memset`?
4. Mematikan `MBEDTLS_ECP_FIXED_POINT_OPTIM` membuat sign 2,35x lebih lambat. Apakah ini mitigasi side-channel? Mengapa?
5. (S) Mengapa optimasi `NIST_OPTIM` pengaruhnya jauh lebih besar daripada `HAVE_ASM`?

### Mikrokontroler
6. Jelaskan masalah *lost wakeup* dan mengapa pola `disable_irq -> cek flag -> WFI -> enable_irq` aman.
7. Mengapa SysTick dimatikan selama tidur dan apa fungsi koreksi `uwTick`?
8. Bagaimana TIM2 dan TIM3 dirangkai menjadi penghitung 32 bit 1 us? Mengapa `hw_us()` membaca tiga kali?
9. Mengapa penerimaan frame memakai DMA, dan apa arti "satu frame = satu bangun CPU"?
10. (S) Mengapa CYCCNT tidak bisa dipakai di Stop mode, dan apa penggantinya?

### Pengukuran
11. (S) Mengapa host memakai jam logger sebagai acuan waktu? Berapa galat HSI yang terukur, dan apa dampaknya pada latensi?
12. Mengapa laju cuplik INA226 yang nyata (495,75/s) penting bagi integrasi energi?
13. Bagaimana dua resistor beban membuktikan bahwa shunt efektif ~0,08 ohm dan bukan ada hambatan seri tetap?
14. Mengapa P_dyn dihitung dengan mengurangi IDLE dari blok yang sama?
15. (S) Mengapa pengacakan per run dibatalkan dan diganti desain blok?
16. Mengapa pin status PB1 diteruskan ke logger padahal firmware sudah menghitung S_cpu sendiri?
17. Apa itu DBGMCU_CR dan bagaimana ia memengaruhi arus tidur?

### Hasil
18. (S) Hitung ambang jenuh `lambda_jenuh` untuk t_sign software 655,5 ms dan hardware 202,2 ms.
19. Pada B (10/s), mengapa software TL hanya ~15 %? Turunkan angkanya dari model.
20. (S) Mengapa P_dyn hardware turun hanya 34 % saat barrage, padahal 77 % saat normal? Metrik apa yang adil?
21. Hitung E_lyn K2 dari P_dyn 5,30 mW dan Theta 0,979 sign/s. Samakah dengan tabel?
22. Mengapa TL tidak diuji dengan t-test?
23. (S) Sebutkan tiga klaim yang TIDAK boleh ditulis di bab kesimpulan.

### Rekayasa
24. (S) Mengapa `firmware.bin` harus identik setelah refaktor, dan apa tiga hal yang diam-diam mengubahnya?
25. Ceritakan satu bug yang hanya terlihat karena ada uji paksa, dan apa pelajarannya.

---

## Jawaban

1. `z = hash(pesan)`; pilih `k`; `R = kG`, `r = x(R) mod n`; `s = k^-1 (z + r d) mod n`. Dua tanda tangan dengan `k` sama:
   `s1 - s2 = k^-1 (z1 - z2)`, sehingga `k` bisa dihitung, lalu `d = (s k - z) / r`. Bocornya `k` satu kali juga cukup
   membuka `d`.
2. Acak murni butuh RNG berkualitas (F103 tidak punya TRNG, entropi dari derau ADC belum tentu baik). RFC 6979 aman
   tanpa RNG tetapi hasil sama tiap sign, rentan fault injection (perbandingan dua hasil salah dan benar). Hedged
   menggabungkan: jika entropi lemah, paling buruk setara RFC 6979; jika baik, hasil berbeda tiap sign.
3. Kompiler boleh membuang `memset` pada memori yang "tidak dibaca lagi" (dead store elimination), meninggalkan
   rahasia di RAM. `zeroize` ditulis agar tidak dapat dioptimasi hilang.
4. Bukan. Opsi itu hanya mematikan *cache* tabel presomputasi G, sehingga tabel dihitung ulang tiap sign. Pemilihan
   entri tabel di Mbed TLS (`ecp_select_comb`) sudah konstan-waktu di kedua mode. Jangan mengklaimnya sebagai mitigasi.
5. Reduksi modulo P-256 terjadi di dalam setiap perkalian. Prima P-256 berbentuk khusus, sehingga NIST_OPTIM cukup memakai
   penjumlahan blok 32 bit. Tanpa itu dipakai pembagian bignum umum (6,6x lebih lambat). Di Cortex-M3 kompiler sudah
   memakai instruksi `umull/umlal`, sehingga assembly manual hanya menambah ~5 %.
6. Bila flag diperiksa lalu interupsi datang sebelum `WFI`, CPU tidur padahal ada kerja menunggu. Dengan PRIMASK=1, interupsi
   tidak dijalankan tetapi tetap pending, dan `WFI` bangun karena pending itu. Baru setelah `enable_irq` ISR dijalankan.
7. Tik SysTick 1 ms akan membangunkan CPU 1000x/s, mengacaukan S_cpu. `uwTick` dikoreksi sebesar durasi tidur agar
   timeout HAL (misalnya `analogRead`) tidak salah menghitung waktu.
8. TIM2 (16 bit) di-prescale ke 1 MHz dan mengirim TRGO saat overflow. TIM3 menjadi budak (mode clock eksternal 1)
   sehingga menghitung overflow TIM2 = 16 bit atas. Pembacaan `hi, lo, hi2`: bila `hi != hi2` ada overflow di tengah baca,
   dan `lo < 4` berarti TRGO belum sempat sampai ke TIM3; ulangi.
9. Tanpa DMA, tiap byte UART menginterupsi CPU (36x per frame). Dengan DMA hanya satu interupsi (Transfer Complete),
   sehingga satu frame membangunkan CPU sekali, sesuai syarat paper.
10. CYCCNT menghitung siklus clock inti; di Stop semua clock 1,8 V berhenti. Penggantinya RTC dengan kristal LSE
    32,768 kHz yang tetap hidup (selisih vs logger 9-20 us per jendela 28,5 s).
11. HSI adalah osilator RC tanpa kristal. Terukur -5.465 ppm (-0,547 %), HSI efektif 47,739 MHz. Latensi dari
    CYCCNT/48 MHz terlalu kecil ~0,55 %: 652,2 ms nominal menjadi 655,5 ms nyata. Logger berkristal HSE jadi acuan.
12. Integrasi trapesium membutuhkan selang antar-cuplikan yang benar. Selang nominal (2,2 ms) berbeda dari nyata
    (2,017 ms, osilator INA226 ~8 % lebih cepat). Memakai nominal membuat energi salah ~8 %.
13. Resistor A (nominal 150) dan B (100) memberi R menurut INA226 181,4 dan 120,2 ohm. Rasio INA226 = 1,509 vs nominal
    1,500: linear. Hambatan seri tetap akan memberi rasio INA/nominal berbeda untuk dua resistor. Rasio konstan (~1,20)
    = faktor skala, jadi shunt efektif = 0,100/1,2 = ~0,083 ohm.
14. Papan sendiri menarik ~11-12 mA tetap. Yang dibandingkan biaya beban kerja. Arus IDLE bergeser antar sesi ~1 %
    mengikuti tegangan USB; mengurangkan IDLE blok yang sama menghapusnya (E_lyn dua sesi beda 0,1-0,4 %).
15. Mengacak per run butuh flash ulang yang butuh ST-Link terpasang. ST-Link mengubah arus terbaca 0,03-0,11 mA dan
    OpenOCD menyalakan DBGMCU_CR. Diganti blok per arsitektur (ST-Link dicabut), dengan urutan A/B diacak di dalam blok
    dan IDLE mengapit blok.
16. Saksi independen. Jika firmware salah menghitung tidurnya, ia akan salah dengan konsisten. Logger menghitung
    dari luar, dengan jam kristal. Hasil: selisih <= 0,004 poin.
17. Register debug MCU. OpenOCD menulis 0x307 (DBG_SLEEP/STOP/STANDBY), sehingga jam inti tetap berjalan saat WFI
    (arus naik 0,2-0,7 mA). Bit itu bertahan sampai power-on reset, jadi setelah flash perlu `run_bench.py nodebug`
    atau cabut-colok daya.
18. `1/0,6555 = 1,53/s` (software), `1/0,2022 = 4,95/s` (hardware); rasio 3,2x.
19. `lambda_eff = 604/60,46 = 9,99/s`; `TL_model = 1/(9,99 x 0,6555) = 0,1527`, jadi 15,27 % (terukur 15,04 %).
    Fisik: 91 sign x 0,6555 s = 59,6 s: CPU hanya sempat 91 sign dari 604 permintaan.
20. Di barrage K4 melayani 3,3x lebih banyak sign (Theta 4,93 vs 1,51), jadi total kerja lebih besar. Metrik adil =
    energi per layanan (E_lyn atau Q_lyn): -80 % di barrage.
21. `E_lyn = P_dyn / Theta = 5,30 / 0,979 = 5,41 mJ`; tabel 5,42 mJ. Selisih dari pembulatan dan Theta.
22. TL dan jumlah layanan deterministik (varians ~0, identik tiap run), sehingga t-test tidak bermakna. Laporkan
    sebagai nilai tetap + validasi model.
23. Contoh: (a) offloading "mencegah" serangan; (b) DS28E38 "tahan tampering/side-channel"; (c) "HW 3x lebih
    cepat" tanpa menyebut konfigurasi comb mati. Juga: kesimpulan apa pun tentang ketahanan fisik.
24. Agar data tahap D tetap sah untuk kode yang di-commit. Gerbang: `python tools/cek_biner.py banding`.
    Tiga hal yang diam-diam menggeser tata letak Flash: `.cpp` bersama di `lib/`, `lib_archive = no`, dan
    urutan path berkas di `src/`. Bandingkan hash `firmware.bin`, bukan ELF (ELF memuat info debug yang berubah).
25. Misalnya bug DMA terkunci pada resync (13 Nov). Uji normal `bad = 0`; baru ketahuan setelah sengaja menggeser frame
    pertama 1 byte (`K1_RX_TEST_MISALIGN`). Pelajaran: uji jalur galat dan jalur langka secara paksa.
