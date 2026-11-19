# Manifes firmware data final varian Stop (18 Nov 2026)

Git HEAD saat manifes: `9b518e27b497`; commit sumber terakhir: 313aadae202dc6a09ea7ee4d55a542a993a160e1 2026-11-18T11:41:33+07:00 Stop: cek idle ketat, p; sumber kotor: False.

| Peran | Env | sha256 firmware.bin | Ukuran (B) | = acuan 16 Nov |
|---|---|---|---|---|
| K1S | host_k1_stop | `2ca6e8d0d86115d09cf9db24b72e511a24f4463b329c0ceaeb76ed485b814a5c` | 47684 | - |
| K2S | host_k2_stop | `5b4e017827e39e17a213bf972b8f3346c15142ec0e02576bb8847680e6ae2f71` | 31308 | - |
| logger | logger | `7ac6b702300d58abb06fcaa2451e7e53801d40424203ee77df1a34cdf3a27e68` | 46812 | True |
| attacker | attacker | `faf092a1fff6f2def7e93b690bfd9eca41a92137d3d33ed6ee66fe77d53cbde9` | 15568 | True |

Dasar kecocokan dengan biner yang di-flash saat sesi (tidak ada sha256 yang dicatat saat itu):
1. Tidak ada berkas sumber (`src lib include boards platformio.ini`) yang lebih baru dari biner host K1S/K2S (dibangun 18 Nov 11:14, sesi data mulai 11:50).
2. Logger dan attacker dibangun ulang dari HEAD dan sha256-nya sama dengan acuan 16 Nov. Host K1S/K2S: pio hanya inkremental (tidak ada yang dikompilasi ulang), jadi ini bukan bukti reproduksi bersih; jalankan `python tools/cek_biner.py simpan host_k1_stop host_k2_stop` bila ingin acuan bersih.
3. Build-id logger 4C364231 tercatat di baris `#RUN,BEGIN` seluruh run.
Keterbatasan: bukti 1 dan 2 tidak menjamin biner yang ada di papan; satu-satunya bukti langsung ialah build-id logger. Untuk sesi berikutnya cetak sha256 firmware yang di-flash di awal sesi.
