# src/attacker — pembangkit permintaan (FINAL)

Env: `attacker` (STM32F030C8, HSI 48 MHz). `main.cpp`: pewaktu TIM3 mengirim frame 36 B lewat USART1 PA9 ke host
(mode A 1 frame/s, B 10 frame/s); konsol CH340 di USART2 (`a`, `b`, `x`, `s`, `f`). Format frame:
`lib/bench_proto/proto_attacker_host.h`. Dijalankan/dihentikan oleh `tools/1_akuisisi/run_bench.py`.

**Mode C (27 Nov 2026, skenario kapasitas ρ = 1):** `p<periode_us>` + Enter (mis. `p116200`) mengirim frame yang sama
setiap periode itu (rentang 1.000–10.000.000 µs; di luar rentang ditolak). Jalur mode A/B tidak diubah.
Biner: sebelum mode C sha256 `faf092a1fff6…` (dipakai data 18 Nov), sesudah `17665901c2fa…` (`biner_acuan.json`).
Desain: `docs/superpowers/specs/2026-11-27-skenario-c-kapasitas-design.md`.
