// ============================================================================
// Protokol ATTACKER -> HOST: frame permintaan autentikasi (pustaka bersama bench_proto, header-only).
//
// SATU-SATUNYA definisi format frame. Dipakai pengirim (src/attacker/main.cpp) dan penerima
// (src/host/dev/tahap_d/bersama/host_rx.h). Mengubah format cukup di sini, lalu bangun ulang attacker DAN host.
//
// Jalur fisik: attacker USART1 PA9 (TX) -> resistor seri ~1 kOhm -> host USART2 PA3 (RX), 8N1,
// ground bersama. Frame dikirim kontigu (36 byte tanpa jeda): penerima DMA menghitungnya sebagai
// satu permintaan = satu bangun CPU.
//
//   offset  isi
//   0       FRAME_SOF (0xA5), penanda awal frame
//   1..2    nomor urut uint16 little-endian (penerima menghitung frame hilang dari selisihnya)
//   3..34   isi 32 byte; TIDAK dipakai host (yang ditandatangani tetap digest konstan K1/K2)
//   35      XOR byte 0..34
// ============================================================================
#pragma once

#define FRAME_BAUD        115200UL   // baud jalur attacker -> host
#define FRAME_SOF         0xA5       // byte penanda awal frame
#define FRAME_PAYLOAD_LEN 32         // isi (pengganti tantangan)
#define FRAME_LEN         36         // 1 SOF + 2 nomor urut + 32 isi + 1 XOR

static_assert(FRAME_LEN == 1 + 2 + FRAME_PAYLOAD_LEN + 1, "tata letak frame tidak konsisten");
