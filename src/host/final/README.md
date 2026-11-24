# src/host/final — firmware host yang mengambil data 18 Nov 2026

| Env | Arsitektur | Sumber |
|---|---|---|
| `host_k1_stop` | K1S: ECDSA P-256 software (Mbed TLS) + Stop | `k1_stop/main.cpp` |
| `host_k2_stop` | K2S: DS28E38 overdrive + tunggu chip di Stop | `k2_stop/main.cpp` + `bersama/ds28e38_od.h` |

Urutan baca: `bersama/stop_common.h` (RTC, Stop, penerima frame) -> `bersama/stop_link.h` (kendali run ke logger) ->
`bersama/stop_window.h` (mesin jendela) -> `main.cpp`. Detail dan diagram: `docs/PETA_ALUR.md`.

Flag `DIAG_SIG`, `DIAG_LONG`, `K1S_AUTO`, `K2S_AUTO`, `DBG_RX` mati di env ini; env yang menyalakannya ada di
`platformio.ini` KELOMPOK 2 (bukan data utama). Kode di sini hanya bergantung pada `../bersama/host_util.h`,
`lib/bench_proto`, dan (K1S) `include/mbedtls_k1_config.h` + `lib/mbedtls`; TIDAK ada yang dari `dev/`.
