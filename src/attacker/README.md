# src/attacker — pembangkit permintaan (FINAL)

Env: `attacker` (STM32F030C8, HSI 48 MHz). `main.cpp`: pewaktu TIM3 mengirim frame 36 B lewat USART1 PA9 ke host
(mode A 1 frame/s, B 10 frame/s); konsol CH340 di USART2 (`a`, `b`, `x`, `s`, `f`). Format frame:
`lib/bench_proto/proto_attacker_host.h`. Dijalankan/dihentikan oleh `tools/1_akuisisi/run_bench.py`.
