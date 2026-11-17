// ============================================================================
// K1 software (Mbed TLS, ECDSA P-256) + STOP MODE  --  LANGKAH 8 = S3 (integrasi K1, 17 Nov 2026)
// Rencana: docs/rencana_deep_sleep.md. Dasar: stop_common.h (RTC, Stop, penerima frame dengan sinkron bit,
// LULUS langkah 7) dan src/host/k1_software/main.cpp (jalur sign: blok kripto di bawah = SALINAN HARFIAH baris
// yang sama di K1 baseline: kunci uji, digest, entropi, HMAC-DRBG hedged, zeroize; K1 baseline tak disentuh).
//
// Fungsi tahap ini satu: jendela pengamatan K1 dengan permintaan NYATA attacker, di mana CPU tidur di STOP
// (bukan WFI) di antara permintaan. Kendali run tetap lewat USART1 ke logger (stop_link.h, protokol sama):
//   ?   -> >ID,K1S,8,<HCLK terukur>,<siap>     S<detik> -> >START, jendela, >END,..., >L..., >DONE
// Beda yang disengaja terhadap K1 WFI:
//   - T_obs dan T_sleep dari RTC (LSE 32,768 kHz, 30,5 us): TIM2/TIM3/TIM4/SysTick mati di Stop.
//   - akhir jendela = alarm RTC (event EXTI17); permintaan = frame attacker (event EXTI3, sinkron bit).
//   - 'aktif' = T_obs - T_sleep (sisa konsistensi = 0 menurut definisi; pembanding independen = PB1 logger:
//     syarat |S_cpu logger - host| <= 0,5 poin di run_bench tetap berlaku).
//   - HCLK diukur terhadap RTC 2 s SETELAH jendela (PB1 rendah) dan dilaporkan di kolom clk_hz, supaya
//     latensi sign dalam ms nyata benar walau HSI -0,5 %.
//   - Jendela aman 5 s + start LSE ~4 s setelah boot: host baru menjawab '?' ~10 s setelah reset.
// Build: pio run -e host_k1_stop    Flash: python tools/flash_stop.py .pio/build/host_k1_stop/firmware.elf
// Uji:   python tools/run_bench.py run A 60 --logger COM18 --attacker COM22
// ============================================================================
#include <Arduino.h>
#include "mbedtls/build_info.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h"
#include "../stop_common.h"

static mbedtls_ecp_group g_grp;
static bool              g_ready = false;
volatile uint32_t g_lse_ok = 0;          // g_lse_ok: syarat 'siap' di stop_link.h


// Digest = SHA-256("Skripsi ECDSA P-256 software vs DS28E38"), sama dengan firmware lain.
static const uint8_t DIGEST[32] = {
  0xD7, 0x2C, 0x10, 0x39, 0x78, 0x44, 0x3C, 0x67, 0xEA, 0xD7, 0x54, 0x29, 0x10, 0xF0, 0xEB, 0xF1,
  0xC8, 0xBA, 0xF6, 0x31, 0x82, 0xC0, 0x7E, 0x06, 0x2B, 0xCC, 0x6F, 0xC2, 0xE9, 0xA8, 0xA5, 0x0E
};

// Kunci privat UJI (statis di Flash; memang model ancaman software-based). Bukan untuk produksi.
static const uint8_t PRIVATE_KEY[32] = {
  0x64, 0x3E, 0xC9, 0x08, 0xDC, 0x6A, 0xF9, 0x93, 0xA3, 0x28, 0x79, 0x13, 0x13, 0x24, 0x1E, 0x69,
  0x3F, 0xAA, 0xD7, 0xA2, 0x33, 0x42, 0x08, 0xF1, 0x2C, 0x99, 0x02, 0x1A, 0xD3, 0xE0, 0x22, 0x3D
};

// ---------------------------------------------------------------------------
// Entropi (H2). Mengumpulkan 32 B: 64 sampel (ADC PA0, CYCCNT, micros) + UID 96-bit chip +
// penghitung sign, dicampur SHA-256. Dipanggil DI LUAR jendela ukur. Bukan TRNG.
static const uint8_t *const CHIP_UID = (const uint8_t *)0x1FFFF7E8UL;   // 12 B, unik per chip
static uint32_t g_signCount = 0;

static int harvest_entropy(uint8_t out[32]) {
  mbedtls_sha256_context h;
  mbedtls_sha256_init(&h);
  int ret = mbedtls_sha256_starts(&h, 0);
  for (int i = 0; i < 64 && ret == 0; i++) {
    uint32_t v[3] = { (uint32_t)analogRead(PA0), cycles(), micros() };
    ret = mbedtls_sha256_update(&h, (const unsigned char *)v, sizeof v);
  }
  if (ret == 0) ret = mbedtls_sha256_update(&h, CHIP_UID, 12);
  if (ret == 0) { g_signCount++; ret = mbedtls_sha256_update(&h, (const unsigned char *)&g_signCount, 4); }
  if (ret == 0) ret = mbedtls_sha256_finish(&h, out);
  mbedtls_sha256_free(&h);                      // free = zeroize
  return ret;
}

// ---------------------------------------------------------------------------
// Kripto: hanya grup P-256 (data publik) yang dimuat sekali saat boot. Kunci privat d TIDAK
// disimpan di RAM global; dimuat per sign dari Flash lalu dihapus (H3).

static bool crypto_init() {
  mbedtls_ecp_group_init(&g_grp);
  return mbedtls_ecdsa_can_do(MBEDTLS_ECP_DP_SECP256R1) &&
         mbedtls_ecp_group_load(&g_grp, MBEDTLS_ECP_DP_SECP256R1) == 0;
}

// Satu operasi "sign": digest 32 B -> r||s (64 B). Hanya fungsi ini yang diukur.
// noinline agar kompiler tidak menggeser kode ke luar kurung CYCCNT.
static int __attribute__((noinline)) sign_digest(const uint8_t *digest, const uint8_t ent[32],
                                                 uint8_t *sig) {
  mbedtls_mpi r, s, d;
  mbedtls_hmac_drbg_context drbg;
  uint8_t seed[96];                             // entropi || d || digest (H1)
  mbedtls_mpi_init(&r); mbedtls_mpi_init(&s); mbedtls_mpi_init(&d);
  mbedtls_hmac_drbg_init(&drbg);

  const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  int ret = md ? 0 : MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
  if (ret == 0) {
    memcpy(seed, ent, 32);
    memcpy(seed + 32, PRIVATE_KEY, 32);
    memcpy(seed + 64, digest, 32);
    ret = mbedtls_hmac_drbg_seed_buf(&drbg, md, seed, sizeof seed);
  }
  if (ret == 0) ret = mbedtls_mpi_read_binary(&d, PRIVATE_KEY, 32);
  // DRBG yang sama dipakai untuk nonce k dan blinding (Mbed TLS memakai f_rng untuk keduanya).
  if (ret == 0) ret = mbedtls_ecdsa_sign(&g_grp, &r, &s, &d, digest, 32,
                                         mbedtls_hmac_drbg_random, &drbg);
  if (ret == 0) ret = mbedtls_mpi_write_binary(&r, sig, 32);
  if (ret == 0) ret = mbedtls_mpi_write_binary(&s, sig + 32, 32);

  mbedtls_platform_zeroize(seed, sizeof seed);
  mbedtls_hmac_drbg_free(&drbg);                // zeroize konteks DRBG
  mbedtls_mpi_free(&d); mbedtls_mpi_free(&r); mbedtls_mpi_free(&s);   // free = zeroize
  return ret;
}

// ---------------------------------------------------------------------------
// Satu operasi penuh yang sama dengan R: entropi + sign. Hasil r||s dibuang.
// g_op_cyc = latensi sign saja (jendela ukur sama dengan R: entropi di luar), dipakai tahap 5.
static uint32_t g_op_cyc = 0;
static int __attribute__((noinline)) one_operation() {
  uint8_t ent[32], sig[64];
  int ret = harvest_entropy(ent);
  const uint32_t t0 = cycles();
  if (ret == 0) ret = sign_digest(DIGEST, ent, sig);
  g_op_cyc = cycles() - t0;
  mbedtls_platform_zeroize(ent, sizeof ent);
  return ret;
}

// ---------------------------------------------------------------------------
// Tahap 8: jalur kendali run USART1 (varian Stop) + mesin jendela bersama (stop_window.h).
#define HL_ARCH  "K1S"
#define HL_READY (g_ready && g_lse_ok)
#include "../stop_link.h"

// Melayani satu permintaan: satu sign (entropi di luar ukur, sign diukur CYCCNT seperti K1 baseline).
static int serve_one(uint32_t *lat) {
  const int r = one_operation();
  *lat = g_op_cyc;
  return r;
}
#include "../stop_window.h"

#define SAFE_MS 5000UL

void setup() {
  usb_off();
  cyccnt_start();
  pins_init();
  hl_init();
  PB1_HI();
  const uint32_t t0 = millis();
  while ((uint32_t)(millis() - t0) < SAFE_MS) {}           // jendela aman SWD (Stop memutus SWD)
  DBGMCU->CR = 0;
  g_ready = crypto_init();
  g_lse_ok = lse_rtc_start() ? 1 : 0;
  rx_hw_init();
  PB1_LO();
  if (g_lse_ok) hl_clk_hz = measure_hclk();
}

#ifdef K1S_AUTO
#ifndef K1S_AUTO_S
#define K1S_AUTO_S      60                                 // lama jendela (s)
#endif
#ifndef K1S_AUTO_GAP_MS
#define K1S_AUTO_GAP_MS 8000                               // jeda sebelum tiap jendela (PB1 rendah, terjaga)
#endif
#endif

void loop() {
#ifdef K1S_AUTO
  // Mode OTOMATIS (17 Nov): dipakai bila jalur logger -> host (PA9 logger -> PA10 host) tidak tersambung.
  // Host memulai sendiri jendela S<K1S_AUTO_S> berulang dan melapor lewat jalur host -> logger yang hidup
  // (>START, >END, >L, >DONE, sama dengan balasan perintah S). Skrip tools/k1s_run.py menyalakan attacker
  // saat >START terlihat dan menganalisis dengan run_bench.analyse.
  if (g_ready && g_lse_ok) {
    const uint32_t t = millis();
    while ((uint32_t)(millis() - t) < K1S_AUTO_GAP_MS) {}
    char cmd[12];
    snprintf(cmd, sizeof cmd, "S%lu", (unsigned long)K1S_AUTO_S);
    hl_txf(LINK_R_START ",%s\n", cmd);
    hl_tx_flush();
    cmd_window(K1S_AUTO_S, 1000, true);
    g_hl_runs++;
    hl_send_result();
    return;
  }
#endif
  hl_service();                                            // perintah dari logger lewat USART1 (hanya di luar jendela)
}
