// ============================================================================
// ECDSA P-256 software TERBERAT di STM32F103 (kelompok K1 & K3, Bab 3 usulan penelitian)
//
// Library : Mbed TLS 3.6.7 (lib/mbedtls), profil "portable C" = semua optimasi opsional MATI
//           (tanpa assembly, tanpa reduksi NIST, tanpa tabel comb, window 2) -> lihat
//           include/mbedtls_heavy_config.h. Ini batas ATAS beban software (worst case);
//           firmware micro-ecc di src/softwaretes/ adalah batas BAWAH (best case).
// Metrik  : latensi (DWT->CYCCNT), RAM (statik, puncak heap, puncak stack per operasi),
//           %S_cpu (waktu di WFI / panjang jendela), serta pin TRIG/SLEEP/WINDOW untuk
//           sinkronisasi profil arus (include/powertest.h).
// Clock   : bawaan STM32duino generic F103C8 + USB = HSI/2 x12 = 48 MHz (sama dengan main.cpp).
// Build   : pio run -e softwaretes_mbedtls -t upload      (cabut ST-Link setelah upload)
//
// Perintah serial (115200, akhiri newline):
//   P                    info sistem, saklar library, uji "DWT tetap berjalan saat WFI"
//   B[n][,gap][|pesan]   latensi + RAM, n kali (default 30), jeda gap ms (tidur WFI) antar
//                        sampel; dengan |pesan, SHA-256(pesan) dihitung dulu di host.
//                        Tiap sampel lalu diverifikasi di board (di luar jendela TRIG).
//   M<pesan>             sekali: SHA-256(pesan) + tanda tangan, cetak digest|r||s|metrik
//   S<hex64>             sekali: tanda tangan digest 32 byte, cetak r||s|metrik
//   N[detik][,periode]   SKENARIO A / K1 (kondisi normal): satu permintaan sah tiap `periode`
//                        ms (default 60 s, 10000 ms); di antaranya CPU tidur (WFI). `periode`
//                        harus > T_sign (estimasi ~5,2 s @ 48 MHz); jika tidak, kolom overrun > 0.
//   A[detik]             SKENARIO B / K3 (barrage attack, DoSl) dengan penyerang EKSTERNAL lewat USB
//                        (tools/window_test.py attack ...): baris `S<hex64>`, satu tanda tangan penuh
//                        per baris valid. Default 60 s. Flow-control ber-ack: firmware mengirim '.'
//                        saat mulai memproses sebuah permintaan, penyerang mengirim 1 baris per '.'.
//                        WAJIB: core STM32duino 4.21 di F103 HARD-FAULT bila antrean RX USB (192 B)
//                        penuh, jadi flood tanpa ack tidak boleh dilakukan.
//   F[detik]             SKENARIO B / K3 dengan penyerang INTERNAL: permintaan selalu tersedia tanpa
//                        jeda (0 ms), tanpa USB sama sekali. Independen dari host; dipakai untuk
//                        pengukuran arus yang bersih. Default 60 s.
// Selama jendela N/F firmware TIDAK mengirim apa pun lewat USB (A hanya mengirim ack '.');
// seluruh hasil dicetak setelah jendela selesai.
// ============================================================================
#include <Arduino.h>
#include "mbedtls/build_info.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/platform.h"
#include "bench.h"
#include "powertest.h"

#define ser SerialUSB

#ifndef ON_DEVICE_VERIFY
#define ON_DEVICE_VERIFY 1     // 0 = hemat flash (~2 KB); verifikasi tetap bisa dilakukan di PC
#endif

#define N_DEFAULT          30       // sampel per skenario (Bab 3.5.2)
#define N_MAX              1000
#define CMD_MAX            255
#define WIN_DEFAULT_S      60       // T_obs (Bab 3.3.1 b.3 dan 3.5.2)
#define PERIOD_DEFAULT_MS  10000    // periode permintaan sah, skenario A (harus > T_sign ~5,2 s)
#define LOG_MAX            128      // jumlah baris latensi per jendela yang disimpan

// ---------------------------------------------------------------------------
// Data uji konstan (identik dengan src/softwaretes/ agar r||s bisa dibandingkan lintas library).
// Digest = SHA-256("Skripsi ECDSA P-256 software vs DS28E38")
static const uint8_t DIGEST[32] = {
  0xD7, 0x2C, 0x10, 0x39, 0x78, 0x44, 0x3C, 0x67, 0xEA, 0xD7, 0x54, 0x29, 0x10, 0xF0, 0xEB, 0xF1,
  0xC8, 0xBA, 0xF6, 0x31, 0x82, 0xC0, 0x7E, 0x06, 0x2B, 0xCC, 0x6F, 0xC2, 0xE9, 0xA8, 0xA5, 0x0E
};

// Kunci privat UJI, statis di Flash: memang model ancaman skema software-based
// (kunci terekspos di memori MCU). JANGAN dipakai untuk produksi.
static const uint8_t PRIVATE_KEY[32] = {
  0x64, 0x3E, 0xC9, 0x08, 0xDC, 0x6A, 0xF9, 0x93, 0xA3, 0x28, 0x79, 0x13, 0x13, 0x24, 0x1E, 0x69,
  0x3F, 0xAA, 0xD7, 0xA2, 0x33, 0x42, 0x08, 0xF1, 0x2C, 0x99, 0x02, 0x1A, 0xD3, 0xE0, 0x22, 0x3D
};

// Kunci publik = PRIVATE_KEY x G, dihitung OFFLINE (tools/p256.py) agar boot tidak menghabiskan
// satu perkalian skalar penuh (beberapa detik pada profil terberat). Format 0x04 || X || Y.
// Salah nilai -> verifikasi di board gagal (kolom verify_ok = 0).
static const uint8_t PUBLIC_KEY[65] = {
  0x04,
  0x96, 0xE6, 0xD7, 0xDD, 0xC5, 0xE8, 0xEF, 0xA4, 0x11, 0x7E, 0x63, 0xBD, 0xDE, 0x6C, 0x28, 0x52,
  0x62, 0xF2, 0x58, 0x5C, 0xE9, 0x6B, 0xFC, 0x9C, 0x58, 0x57, 0xF9, 0xBC, 0x5B, 0x2D, 0xEA, 0xF7,
  0x29, 0x4D, 0x5C, 0xB0, 0x52, 0x19, 0x50, 0x74, 0x0B, 0x76, 0xE9, 0x8B, 0x7A, 0x91, 0x45, 0xB1,
  0x80, 0x3F, 0x97, 0xD1, 0x63, 0xC2, 0x88, 0x8F, 0x64, 0xA6, 0xBE, 0x0E, 0xB1, 0xBB, 0xAD, 0x99
};

// ---------------------------------------------------------------------------
// RNG untuk nonce k, blinding, dan randomisasi koordinat Z. STM32F103 tidak punya TRNG:
// benih dari derau ADC saat boot, diperluas xorshift128. Murah (tidak mengubah latensi)
// tetapi TIDAK aman secara kriptografis; cukup untuk benchmark. Konvensi Mbed TLS: return 0 = sukses.
static uint32_t rs[4];

static uint32_t xorshift128() {
  uint32_t t = rs[3], s = rs[0];
  rs[3] = rs[2]; rs[2] = rs[1]; rs[1] = s;
  t ^= t << 11; t ^= t >> 8;
  return rs[0] = t ^ s ^ (s >> 19);
}

static int rng_cb(void *, unsigned char *dest, size_t size) {
  while (size) {
    uint32_t r = xorshift128();
    for (int i = 0; i < 4 && size; i++, size--) { *dest++ = (uint8_t)r; r >>= 8; }
  }
  return 0;
}

static void rng_seed() {
  for (int i = 0; i < 4; i++) {
    uint32_t v = 0;
    for (int j = 0; j < 32; j++) v = (v << 1 | v >> 31) ^ analogRead(PA0) ^ (cycles() << j);
    rs[i] = v ^ (0x9E3779B9UL * (i + 1));
  }
  if (!(rs[0] | rs[1] | rs[2] | rs[3])) rs[0] = 1;
}

// ---------------------------------------------------------------------------
// Pelacak heap: Mbed TLS mengalokasi bignum lewat calloc/free. Header 8 byte menyimpan ukuran
// agar puncak alokasi per operasi bisa dilaporkan (kolom heap_peak). Biayanya beberapa siklus
// per alokasi, diabaikan terhadap puluhan juta siklus per tanda tangan.
static uint32_t g_heap_cur = 0, g_heap_peak = 0;

static void *trk_calloc(size_t n, size_t sz) {
  size_t bytes = n * sz;
  uint32_t *p = (uint32_t *)calloc(1, bytes + 8);
  if (!p) return NULL;
  p[0] = (uint32_t)bytes;
  g_heap_cur += (uint32_t)bytes;
  if (g_heap_cur > g_heap_peak) g_heap_peak = g_heap_cur;
  return p + 2;
}

static void trk_free(void *ptr) {
  if (!ptr) return;
  uint32_t *p = (uint32_t *)ptr - 2;
  g_heap_cur -= p[0];
  free(p);
}

// ---------------------------------------------------------------------------
// Kripto: grup P-256, kunci privat d, kunci publik Q dimuat sekali saat boot.
static mbedtls_ecp_group g_grp;
static mbedtls_mpi       g_d;
static mbedtls_ecp_point g_Q;
static bool g_ready = false, g_warm = false, g_dwt_in_sleep = false;

static bool crypto_init() {
  mbedtls_platform_set_calloc_free(trk_calloc, trk_free);
  mbedtls_ecp_group_init(&g_grp); mbedtls_mpi_init(&g_d); mbedtls_ecp_point_init(&g_Q);
  return mbedtls_ecp_group_load(&g_grp, MBEDTLS_ECP_DP_SECP256R1) == 0
      && mbedtls_mpi_read_binary(&g_d, PRIVATE_KEY, 32) == 0
      && mbedtls_ecp_point_read_binary(&g_grp, &g_Q, PUBLIC_KEY, sizeof(PUBLIC_KEY)) == 0
      && mbedtls_ecp_check_pubkey(&g_grp, &g_Q) == 0;
}

// digest 32 byte -> r||s (64 byte). Inilah satu-satunya operasi yang setara dengan satu
// perintah "sign" ke DS28E38; semua pengukuran latensi/daya membungkus fungsi ini.
static int sign_digest(const uint8_t *digest, uint8_t *sig) {
  mbedtls_mpi r, s;
  mbedtls_mpi_init(&r); mbedtls_mpi_init(&s);
  int ret = mbedtls_ecdsa_sign(&g_grp, &r, &s, &g_d, digest, 32, rng_cb, NULL);
  if (ret == 0) ret = mbedtls_mpi_write_binary(&r, sig, 32);
  if (ret == 0) ret = mbedtls_mpi_write_binary(&s, sig + 32, 32);
  mbedtls_mpi_free(&r); mbedtls_mpi_free(&s);
  return ret;
}

#if ON_DEVICE_VERIFY
static bool verify_sig(const uint8_t *digest, const uint8_t *sig) {
  mbedtls_mpi r, s;
  mbedtls_mpi_init(&r); mbedtls_mpi_init(&s);
  bool ok = mbedtls_mpi_read_binary(&r, sig, 32) == 0 && mbedtls_mpi_read_binary(&s, sig + 32, 32) == 0
         && mbedtls_ecdsa_verify(&g_grp, digest, 32, &g_Q, &r, &s) == 0;
  mbedtls_mpi_free(&r); mbedtls_mpi_free(&s);
  return ok;
}
#endif

// Tanda tangan pertama mengalokasi heap newlib dari nol (sbrk membesar). Itu merusak lukisan
// stack di bench.h bila terjadi di dalam operasi terukur, jadi dilakukan sekali sebelum
// pengukuran pertama dan tidak diukur.
static void ensure_warm() {
  if (g_warm) return;
  uint8_t sig[64];
  ser.println("# Pemanasan: satu tanda tangan awal (alokasi heap pertama), tidak diukur...");
  sign_digest(DIGEST, sig);
  g_warm = true;
}

// ---------------------------------------------------------------------------
// Utilitas cetak. Tanpa floating point (soft-float double menambah beberapa KB Flash).
static void print_hex(const uint8_t *p, size_t n) {
  static const char H[] = "0123456789ABCDEF";
  for (size_t i = 0; i < n; i++) { ser.write(H[p[i] >> 4]); ser.write(H[p[i] & 0xF]); }
}

// cetak v/1000 dengan 3 desimal (v dalam satuan 0,001)
static void print_milli(uint64_t v) {
  ser.printf("%lu.%03lu", (unsigned long)(v / 1000), (unsigned long)(v % 1000));
}

static uint64_t cyc_to_us64(uint64_t c) { return c * 1000000ULL / SystemCoreClock; }
static void print_ms(uint64_t cyc)      { print_milli(cyc_to_us64(cyc)); }   // siklus -> ms, 3 desimal

static int hex_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static bool parse_hex(const char *s, uint8_t *out, int n) {
  for (int i = 0; i < n; i++) {
    int hi = hex_nibble(s[2*i]), lo = hi < 0 ? -1 : hex_nibble(s[2*i+1]);
    if (lo < 0) return false;
    out[i] = (hi << 4) | lo;
  }
  return true;
}

static uint32_t parse_u32(const char *&p, uint32_t def) {
  if (*p < '0' || *p > '9') return def;
  uint32_t v = 0;
  while (*p >= '0' && *p <= '9') v = v * 10 + (uint32_t)(*p++ - '0');
  return v;
}

// ---------------------------------------------------------------------------
// Statistik berjalan, bilangan bulat (satuan mikrodetik). Aman hingga n ~ 1000 sampel <= 3 s.
typedef struct { uint32_t n; uint64_t sum, sumsq; uint32_t mn, mx; } Stat;

static void stat_init(Stat &s) { s.n = 0; s.sum = s.sumsq = 0; s.mn = 0xFFFFFFFF; s.mx = 0; }

static void stat_add(Stat &s, uint32_t us) {
  s.n++; s.sum += us; s.sumsq += (uint64_t)us * us;
  if (us < s.mn) s.mn = us;
  if (us > s.mx) s.mx = us;
}

static uint32_t isqrt64(uint64_t x) {
  uint64_t r = 0, bit = 1ULL << 62;
  while (bit > x) bit >>= 2;
  while (bit) { if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; } else r >>= 1; bit >>= 2; }
  return (uint32_t)r;
}

static void stat_print(const char *label, const Stat &s) {
  if (!s.n) return;
  uint64_t mean = s.sum / s.n;
  uint64_t var  = s.n > 1 ? (s.sumsq - s.sum * s.sum / s.n) / (s.n - 1) : 0;
  ser.printf("# %-8s ms: mean=", label); print_milli(mean);
  ser.print(" sd=");  print_milli(isqrt64(var));
  ser.print(" min="); print_milli(s.mn);
  ser.print(" max="); print_milli(s.mx); ser.println();
}

// ---------------------------------------------------------------------------
// Satu operasi terukur: [SHA-256(pesan)] lalu sign. Pin TRIG HIGH tepat membungkus sign saja.
typedef struct {
  uint32_t cyc_hash;    // siklus SHA-256 pesan (0 jika digest diberikan langsung)
  uint32_t cyc_sign;    // siklus sign_digest (jendela yang sama dengan pin TRIG)
  uint32_t stack;       // byte stack terdalam yang dipakai hash + sign (hanya jika paint)
  uint32_t heap_peak;   // puncak byte heap yang dialokasikan selama sign
  bool     ok;
} OpSample;

static uint32_t __attribute__((noinline)) timed_sign(const uint8_t *digest, uint8_t *sig, bool *ok) {
  pt_set(PT_BIT_TRIG);
  uint32_t t0 = cycles();
  int r = sign_digest(digest, sig);
  uint32_t t1 = cycles();
  pt_clr(PT_BIT_TRIG);
  *ok = (r == 0);
  return t1 - t0;
}

// msg != NULL: digest dihitung dari pesan ke `digest`; msg == NULL: `digest` dipakai apa adanya
static OpSample __attribute__((noinline))
measure_op(const char *msg, size_t len, uint8_t *digest, uint8_t *sig, bool paint) {
  OpSample s = {0, 0, 0, 0, false};
  if (paint) stack_paint();
  uint32_t sp0 = get_sp();
  uint32_t heap0 = g_heap_cur;
  g_heap_peak = heap0;

  uint32_t t0 = cycles();
  if (msg) sha256_msg(msg, len, digest);
  uint32_t t1 = cycles();
  s.cyc_hash = msg ? t1 - t0 : 0;
  s.cyc_sign = timed_sign(digest, sig, &s.ok);

  s.stack     = paint ? sp0 - stack_low_water() : 0;
  s.heap_peak = g_heap_peak - heap0;
  return s;
}

// ---------------------------------------------------------------------------
static bool no_work()        { return false; }
static bool pending_rx()     { return ser.available() > 0; }
static uint32_t g_next_ms;
static bool pending_period() { return (int32_t)(millis() - g_next_ms) >= 0; }

// Uji sendiri: apakah DWT->CYCCNT tetap bertambah ketika core berada di WFI? %S_cpu dan T_obs
// bergantung pada ini. Pada STM32F1, FCLK tetap hidup di mode Sleep, jadi seharusnya "ya";
// bila "TIDAK", hasil %S_cpu tidak valid dan harus diganti pewaktu perangkat keras.
static void selftest_wfi() {
  uint32_t m0 = millis();
  while (millis() == m0) { }
  m0 = millis();
  uint32_t c0 = cycles();
  uint64_t s0 = pt_sleep_cyc;
  while ((uint32_t)(millis() - m0) < 20) pt_idle(no_work);
  uint32_t dc = cycles() - c0;
  uint64_t ds = pt_sleep_cyc - s0;
  uint64_t expect = (uint64_t)SystemCoreClock / 50;           // 20 ms
  g_dwt_in_sleep = dc > expect * 95 / 100 && dc < expect * 105 / 100 && ds * 100 > (uint64_t)dc * 80;
}

static void print_sysinfo() {
  ser.printf("# MCU clock     : %lu Hz\n", (unsigned long)SystemCoreClock);
  ser.printf("# Library       : Mbed TLS %s, ECDSA secp256r1 (NIST P-256)\n", MBEDTLS_VERSION_STRING);
#if defined(MBEDTLS_PROFILE_DEFAULT)
  ser.println("# Profil        : DEFAULT (optimasi upstream aktif) = batas BAWAH Mbed TLS");
#else
  ser.println("# Profil        : HEAVY (portable C, semua optimasi mati) = batas ATAS beban software");
#endif
#if defined(MBEDTLS_HAVE_ASM)
  const int asm_on = 1;
#else
  const int asm_on = 0;
#endif
#if defined(MBEDTLS_ECP_NIST_OPTIM)
  const int nist_on = 1;
#else
  const int nist_on = 0;
#endif
  ser.printf("# Saklar        : HAVE_ASM=%d NIST_OPTIM=%d FIXED_POINT_OPTIM=%d WINDOW_SIZE=%d\n",
             asm_on, nist_on, (int)MBEDTLS_ECP_FIXED_POINT_OPTIM, (int)MBEDTLS_ECP_WINDOW_SIZE);
  ser.printf("# RAM total     : %lu B\n", (unsigned long)((uint32_t)&_estack - (uint32_t)&_sdata));
  ser.printf("# RAM statik    : %lu B (.data+.bss seluruh firmware)\n", (unsigned long)ram_static_bytes());
  ser.printf("# Heap terpakai : %lu B (sbrk), %lu B dialokasi Mbed TLS saat ini\n",
             (unsigned long)heap_bytes(), (unsigned long)g_heap_cur);
  ser.printf("# DWT di WFI    : %s\n", g_dwt_in_sleep ? "ya (CYCCNT tetap berjalan saat tidur)" : "TIDAK - %S_cpu TIDAK VALID");
  ser.printf("# Pin ukur      : TRIG=PB0 SLEEP=PB1 WINDOW=PB10\n");
  ser.print("# Public key    : "); print_hex(PUBLIC_KEY + 1, 64); ser.println();
}

// ---------------------------------------------------------------------------
// B: benchmark latensi + RAM
static void run_benchmark(uint32_t n, uint32_t gap_ms, const char *msg) {
  Stat st_hash, st_sign, st_total;
  stat_init(st_hash); stat_init(st_sign); stat_init(st_total);
  uint32_t stack_min = 0xFFFFFFFF, stack_max = 0, heap_min = 0xFFFFFFFF, heap_max = 0;
  uint32_t n_ok = 0, n_valid = 0;
  uint8_t digest[32], sig[64];
  size_t len = msg ? strlen(msg) : 0;
  if (!msg) memcpy(digest, DIGEST, 32);

  ensure_warm();
  print_sysinfo();
  if (msg) ser.printf("# Pesan         : \"%s\" (%u B), di-hash SHA-256 di host\n", msg, (unsigned)len);
  else     ser.println("# Pesan         : (tidak ada) digest konstan, tanpa hash");
  ser.printf("# Benchmark N=%lu, jeda antar sampel=%lu ms (verifikasi di luar latensi dan pin TRIG)\n",
             (unsigned long)n, (unsigned long)gap_ms);
  ser.println("i,cyc_hash,cyc_sign,ms_hash,ms_sign,ms_total,stack_bytes,heap_peak,sign_ok,verify_ok");

  for (uint32_t i = 0; i < n; i++) {
    OpSample s = measure_op(msg, len, digest, sig, true);
    bool valid = false;
#if ON_DEVICE_VERIFY
    valid = s.ok && verify_sig(digest, sig);
#endif
    stat_add(st_hash, cyc_to_us(s.cyc_hash));
    stat_add(st_sign, cyc_to_us(s.cyc_sign));
    stat_add(st_total, cyc_to_us(s.cyc_hash) + cyc_to_us(s.cyc_sign));
    if (s.stack < stack_min) stack_min = s.stack;
    if (s.stack > stack_max) stack_max = s.stack;
    if (s.heap_peak < heap_min) heap_min = s.heap_peak;
    if (s.heap_peak > heap_max) heap_max = s.heap_peak;
    n_ok += s.ok; n_valid += valid;

    ser.printf("%lu,%lu,%lu,", (unsigned long)(i + 1), (unsigned long)s.cyc_hash, (unsigned long)s.cyc_sign);
    print_ms(s.cyc_hash); ser.write(','); print_ms(s.cyc_sign); ser.write(','); print_ms((uint64_t)s.cyc_hash + s.cyc_sign);
    ser.printf(",%lu,%lu,%d,%d\n", (unsigned long)s.stack, (unsigned long)s.heap_peak, (int)s.ok, (int)valid);

    if (gap_ms) {                         // jeda tidur: memisahkan tiap tanda tangan pada profil arus
      g_next_ms = millis() + gap_ms;
      while (!pending_period()) pt_idle(pending_period);
    }
  }

  ser.print("# Digest        : "); print_hex(digest, 32); ser.println();
  stat_print("T_hash", st_hash);
  stat_print("T_sign", st_sign);
  stat_print("T_total", st_total);
  ser.printf("# Stack operasi : min=%lu max=%lu B\n", (unsigned long)stack_min, (unsigned long)stack_max);
  ser.printf("# Heap puncak   : min=%lu max=%lu B (dialokasi Mbed TLS selama sign)\n",
             (unsigned long)heap_min, (unsigned long)heap_max);
#if ON_DEVICE_VERIFY
  ser.printf("# Valid         : sign %lu/%lu, verify %lu/%lu\n", (unsigned long)n_ok, (unsigned long)n,
             (unsigned long)n_valid, (unsigned long)n);
#else
  ser.printf("# Valid         : sign %lu/%lu, verify dinonaktifkan (ON_DEVICE_VERIFY=0)\n", (unsigned long)n_ok, (unsigned long)n);
#endif
  ser.println("# SELESAI");
}

// Satu operasi: cetak [digest|] r||s | cyc_hash,cyc_sign,ms_hash,ms_sign,ms_total,stack
static void sign_once(const char *msg, uint8_t *digest) {
  ensure_warm();
  uint8_t sig[64];
  OpSample s = measure_op(msg, msg ? strlen(msg) : 0, digest, sig, true);
  if (!s.ok) { ser.println("-1"); return; }
  if (msg) { print_hex(digest, 32); ser.write('|'); }
  print_hex(sig, 64);
  ser.printf("|%lu,%lu,", (unsigned long)s.cyc_hash, (unsigned long)s.cyc_sign);
  print_ms(s.cyc_hash); ser.write(','); print_ms(s.cyc_sign); ser.write(','); print_ms((uint64_t)s.cyc_hash + s.cyc_sign);
  ser.printf(",%lu\n", (unsigned long)s.stack);
}

// ---------------------------------------------------------------------------
// N / A: jendela pengamatan T_obs (Bab 3.5.2 skenario A dan B)
static uint32_t g_log_ms[LOG_MAX];    // waktu mulai permintaan (ms sejak awal jendela)
static uint32_t g_log_cyc[LOG_MAX];   // siklus sign

static bool parse_request(const char *l, int n, uint8_t *digest) {
  uint8_t tmp[32];
  if (n != 65 || l[0] != 'S' || !parse_hex(l + 1, tmp, 32)) return false;
  memcpy(digest, tmp, 32);
  return true;
}

enum { W_NORMAL = 0, W_USB = 1, W_INTERNAL = 2 };

static void run_window(int mode, uint32_t secs, uint32_t period_ms) {
  const bool normal = (mode == W_NORMAL), usb = (mode == W_USB);
  ensure_warm();
  ser.printf("# SKENARIO %s: T_obs=%lu s", normal ? "A (kondisi normal)" : "B (barrage attack / DoSl)", (unsigned long)secs);
  if (usb)          ser.println(", penyerang eksternal lewat USB (1 baris S<hex64> per ack '.')");
  else if (!normal) ser.println(", penyerang internal: permintaan selalu tersedia, tanpa jeda");
  else              ser.printf(", 1 permintaan sah tiap %lu ms\n", (unsigned long)period_ms);
  ser.println("# MULAI (tidak ada keluaran serial selama jendela, kecuali ack '.' pada mode A)");
  { uint32_t t = millis(); while (millis() - t < 100) { } }   // beri waktu USB mengosongkan antrean TX

  Stat st; stat_init(st);
  uint32_t n_sign = 0, n_bad = 0, n_overrun = 0, n_fail = 0;
  uint64_t sign_cyc_total = 0;
  uint8_t digest[32], sig[64];
  memcpy(digest, DIGEST, 32);
  char line[80]; int ll = 0;
  const uint32_t heap_base = g_heap_cur;
  g_heap_peak = heap_base;            // puncak heap dihitung hanya selama jendela ini

  pt_win_begin();
  const uint32_t ms0 = millis();
  const uint32_t t_end = ms0 + secs * 1000UL;
  g_next_ms = ms0;

  while ((int32_t)(millis() - t_end) < 0) {
    bool do_req = false;
    if (normal) {
      if (pending_period()) do_req = true; else pt_idle(pending_period);
    } else if (!usb) {
      do_req = true;                                   // penyerang internal: selalu ada permintaan
    } else if (ser.available() > 0) {
      int c = ser.read();
      if (c == '\n') { do_req = parse_request(line, ll, digest); if (!do_req && ll) n_bad++; ll = 0; }
      else if (c != '\r') { if (ll < (int)sizeof(line)) line[ll++] = (char)c; }
    } else {
      pt_idle(pending_rx);
    }

    if (do_req) {
      uint32_t t_req = millis() - ms0;
      if (usb) ser.write('.');                         // ack: penyerang boleh mengirim 1 baris lagi
      bool ok;
      uint32_t cyc = timed_sign(digest, sig, &ok);
      if (!ok) n_fail++;
      if (n_sign < LOG_MAX) { g_log_ms[n_sign] = t_req; g_log_cyc[n_sign] = cyc; }
      n_sign++;
      sign_cyc_total += cyc;
      stat_add(st, cyc_to_us(cyc));
      if (normal) { g_next_ms += period_ms; if (pending_period()) n_overrun++; }
    }
  }
  PtWindow w = pt_win_end();

  if (usb) {                          // buang sisa baris penyerang yang masih di buffer USB
    uint32_t last = millis();
    while (millis() - last < 1000) { if (ser.available() > 0) { ser.read(); last = millis(); } }
  }

  ser.println("# SELESAI JENDELA");
  ser.println("i,t_start_ms,cyc_sign,ms_sign");
  uint32_t shown = n_sign < LOG_MAX ? n_sign : LOG_MAX;
  for (uint32_t i = 0; i < shown; i++) {
    ser.printf("%lu,%lu,%lu,", (unsigned long)(i + 1), (unsigned long)g_log_ms[i], (unsigned long)g_log_cyc[i]);
    print_ms(g_log_cyc[i]); ser.println();
  }
  if (n_sign > LOG_MAX) ser.printf("# (hanya %d baris pertama dicetak dari %lu; statistik memakai semuanya)\n", LOG_MAX, (unsigned long)n_sign);

  ser.printf("# Mode           : %s\n", normal ? "A kondisi normal" : usb ? "B barrage attack (DoSl), penyerang USB" : "B barrage attack (DoSl), penyerang internal");
  ser.print("# T_obs (DWT)    : "); print_ms(w.total_cyc); ser.printf(" ms  (millis: %lu ms)\n", (unsigned long)w.ms_millis);
  uint64_t dwt_ms = cyc_to_us64(w.total_cyc) / 1000;
  if (dwt_ms + dwt_ms / 100 + 3 < w.ms_millis || w.ms_millis + w.ms_millis / 100 + 3 < dwt_ms)
    ser.println("# PERINGATAN: T_obs DWT dan millis() berbeda >1%; CYCCNT kemungkinan berhenti saat WFI");
  ser.printf("# Permintaan     : ditandatangani=%lu gagal=%lu ditolak(format)=%lu overrun=%lu\n",
             (unsigned long)n_sign, (unsigned long)n_fail, (unsigned long)n_bad, (unsigned long)n_overrun);
  ser.print("# Tidur WFI      : "); print_ms(w.sleep_cyc); ser.print(" ms   S_cpu="); print_milli(pt_sleep_milli_pct(w));
  ser.printf(" %%   (WFI x%lu)\n", (unsigned long)w.wfi_count);
  uint64_t active = w.total_cyc - w.sleep_cyc;
  ser.print("# Aktif          : "); print_ms(active); ser.print(" ms   (di dalam sign: "); print_ms(sign_cyc_total);
  ser.print(" ms = "); print_milli(w.total_cyc ? sign_cyc_total * 100000ULL / w.total_cyc : 0); ser.println(" % dari T_obs)");
  stat_print("T_sign", st);
  ser.printf("# Heap puncak    : %lu B (dialokasi Mbed TLS selama jendela)\n", (unsigned long)(g_heap_peak - heap_base));
  ser.println("# SELESAI");
}

// ---------------------------------------------------------------------------
void setup() {
  ser.begin(115200);
  bool dwt_ok = cyccnt_init();
  pt_pins_init();
  rng_seed();
  g_ready = crypto_init();
  selftest_wfi();

  for (uint32_t t = millis(); !ser && (millis() - t) < 10000; ) { }
  ser.println("SOFTWARETES MBEDTLS ECDSA P-256 siap. Perintah: P | B[n][,gap][|pesan] | M<pesan> | S<hex64> | N[detik][,periode] | A[detik] | F[detik]");
  if (!dwt_ok)  ser.println("# PERINGATAN: DWT CYCCNT tidak berjalan");
  if (!g_ready) ser.println("# PERINGATAN: gagal memuat grup P-256 / kunci publik");
  if (!g_dwt_in_sleep) ser.println("# PERINGATAN: CYCCNT tidak bertambah saat WFI; %S_cpu tidak valid (lihat perintah P)");
}

void loop() {
  static char line[CMD_MAX];
  static int n = 0;
  if (ser.available() <= 0) return;
  char c = ser.read();
  if (c == '\r') return;
  if (c != '\n') { if (n < CMD_MAX - 1) line[n++] = c; return; }
  line[n] = '\0'; n = 0;
  if (!g_ready && line[0]) { ser.println("-1"); return; }

  switch (line[0]) {
    case 'B': {
      char *msg = strchr(line, '|');
      if (msg) *msg++ = '\0';
      const char *p = line + 1;
      uint32_t k = parse_u32(p, N_DEFAULT), gap = 0;
      if (*p == ',') { p++; gap = parse_u32(p, 0); }
      if (k < 1 || k > N_MAX) { ser.println("-1"); break; }
      run_benchmark(k, gap, msg);
      break;
    }
    case 'M': { uint8_t d[32]; sign_once(line + 1, d); break; }
    case 'S': {
      uint8_t d[32];
      if (!parse_hex(line + 1, d, 32)) { ser.println("-1"); break; }
      sign_once(nullptr, d);
      break;
    }
    case 'N': {
      const char *p = line + 1;
      uint32_t secs = parse_u32(p, WIN_DEFAULT_S), period = PERIOD_DEFAULT_MS;
      if (*p == ',') { p++; period = parse_u32(p, PERIOD_DEFAULT_MS); }
      if (secs < 1 || secs > 3600 || period < 1) { ser.println("-1"); break; }
      run_window(W_NORMAL, secs, period);
      break;
    }
    case 'A':
    case 'F': {
      const char *p = line + 1;
      uint32_t secs = parse_u32(p, WIN_DEFAULT_S);
      if (secs < 1 || secs > 3600) { ser.println("-1"); break; }
      run_window(line[0] == 'A' ? W_USB : W_INTERNAL, secs, 0);
      break;
    }
    case 'P': print_sysinfo(); break;
    case '\0': break;
    default: ser.println("-1");
  }
}
