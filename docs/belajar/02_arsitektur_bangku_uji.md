# 02. Arsitektur Bangku Uji

## Gambaran besar

Tiga board, masing-masing dengan satu tugas supaya pengukuran tidak saling mengganggu.

```
                PC (tools/1_akuisisi/run_bench.py)
        USB CDC (CSV)  │            │ CH340 (konsol a/b/x)
                       │            │
        ┌──────────────┴───┐   ┌────┴─────────────┐
        │ LOGGER F103      │   │ ATTACKER F030C8  │
        │ HSE 72 MHz       │   │ TIM3: 1 atau     │
        │ INA226 (I2C)     │   │ 10 frame/detik   │
        │ PA0 tangkap tepi │   │ PA9 ──[1k/10k]──┐│
        │ PA9/PA10 kendali │   └─────────────────┼┘
        └───┬───────┬──────┘                     │
            │       │ USART1 (>START, >END...)   │ USART2 (frame 36 B)
       I2C  │       │                            │
   ┌────────┴──┐    │    ┌───────────────────────┴───┐
   │ INA226    │    └───►│ HOST F103 (K1 atau K2)    │
   │ shunt di  │ catu    │ HSI 48 MHz, PB1 = status  │
   │ jalur 5V  ├────────►│ PA15 ── DS28E38 (K2)      │
   └───────────┘         └───────────────────────────┘
           PB1 host ──► PA0 logger (pin status CPU)
```

| Board | Tugas | Mengapa dipisah |
|---|---|---|
| **Host** (yang diuji) | menandatangani permintaan, tidur di sela | tidak boleh melakukan hal lain agar ukuran bersih |
| **Logger** | membaca INA226, mencatat tepi pin status, mengendalikan run | jam kristal presisi; punya acuan waktu independen |
| **Attacker** | membangkitkan permintaan (A 1/s, B 10/s) | beban dibuat konstan dan tidak ikut membebani host |

## Mengapa pin status PB1

Host menaikkan PB1 saat CPU aktif dan menurunkannya sebelum `WFI`. Logger menangkap tepi dengan timer input-capture
(TIM2, 72 MHz, diperluas ke 64 bit dengan pencacah overflow). Hasilnya **pembanding independen** untuk `S_cpu`:
firmware menghitung tidur dengan timernya sendiri, logger menghitung dari luar. Hasil: selisih <= 0,004 poin.

**Prinsip rekayasa.** Jangan percaya satu alat ukur yang dikendalikan oleh objek yang diukur. Tambahkan saksi independen.

## Alur satu run (60 detik)

1. PC memerintahkan logger (lewat nilai baud, karena jalur OUT USB klon rusak).
2. Logger mulai merekam INA226 + tepi PA0 sekitar 100 ms sebelum jendela, lalu mengirim `S60` ke host via USART1.
3. Host mengirim `>START`, lalu menaikkan PB1 (t0) dan memulai jendela. PC baru menjalankan attacker (`a`/`b`) setelah
   `>START` terlihat. Frame pertama tiba sekitar 1 s (A) atau 0,1 s (B) setelah t0, sehingga `N_dtg` 59 (A) dan sekitar 603 (B).
4. Selama 60 s host tidur di `WFI` dan bangun per permintaan: PB1 naik, sign, catat latensi di RAM, PB1 turun, tidur.
5. Akhir jendela: host mengirim `>END,<23 kolom>`, `>L,<latensi>` (10 per baris), `>DONE`.
   Latensi disimpan di RAM dan dikirim **setelah** jendela agar transmisi tidak mengganggu pengukuran.
6. Logger mengalirkan CSV ke PC: `s` (cuplikan INA), `e` (tepi), `h` (baris host), `#SUM`, `#RUN,END`.
7. PC menganalisis: keutuhan, T_obs dari tepi PB1, energi trapesium, metrik layanan.

## Protokol, dua lapis

**Attacker ke host** (`proto_attacker_host.h`): frame biner 36 B, 115200 baud, lewat resistor seri.
Resistor seri melindungi pin saat bongkar-pasang dengan beda catu/ground. Pada 17 Nov diganti 10 kohm.

**Host dan logger** (`proto_host_logger.h`): teks ASCII per baris.
`?` (identitas), `S<detik>` (permintaan nyata), `W<detik>,<p>` (sintetis). Balasan: `>ID`, `>START`, `>END`, `>L`,
`>DONE`, `>ERR`. Teks dipilih supaya mudah di-debug dengan terminal.

**Satu definisi, banyak pemakai.** Format dideklarasikan sekali di `lib/bench_proto/` (header-only), dipakai attacker,
host, dan logger. Mengubah format cukup di satu tempat, lalu bangun ulang semua yang memakainya.

## Variabel kontrol (agar K1 vs K2 adil)

| Dikendalikan | Cara |
|---|---|
| Tantangan sama | digest SHA-256 konstan 32 B |
| Beban sama | satu flag `g_pending`, tanpa antrean, permintaan yang bertumpuk digabung |
| Papan sama | satu board fisik, firmware diganti |
| Jam sama | 48 MHz HSI kedua-duanya, dikoreksi dengan jam logger |
| Titik ukur sama | input 5 V USB-C lewat INA226 |
| Pengaruh debug | ST-Link dicabut, `DBGMCU_CR = 0`, power-on reset |
| LED | PC13 dibiarkan mati |

**Kebijakan "satu flag" harus disebut di Bab 3.** Dengan antrean, TL bisa lebih tinggi tetapi waktu tunggu bertambah.
Definisi "terlewat" = permintaan yang tiba saat host sibuk dan digabung.

## Hal yang "bocor" lewat kabel (pelajaran perangkat keras)

| Gejala | Penyebab | Pelajaran |
|---|---|---|
| Duty 0 %, HIGH hanya 2 us di uji L4 (15 Nov) | kabel PB1 host masih menancap di PA0, melawan PWM uji | cek kabel fisik dulu sebelum menyalahkan kode |
| Arus naik +0,78 mA di semua keadaan | host tersangkut setengah-reset oleh daya balik lewat jalur sinyal (rail 2,37 V saat USB dicabut) | papan bisa "berdaya" lewat pin I/O dan dioda proteksi |
| `read version failed` pada ST-Link | kabel 3V3 putus; pin itu dipakai ST-Link mengukur tegangan target | baca makna tiap kabel |
| Dugaan "dongle rusak" salah | yang bermasalah set kabel, bukan dongle | tukar satu variabel per percobaan |

**Prosedur POR bersih** (17 Nov): cabut ST-Link, cabut USB-C, lepas GND host-logger, tunggu 10 s, sambung GND **dulu**,
baru colok USB-C.
