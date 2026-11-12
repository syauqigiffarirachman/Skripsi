// ============================================================================
// K1 software (Mbed TLS, ECDSA P-256) — TAHAP 1: BASE STANDAR
//
// Aturan "build from base": tahap ini HANYA membuktikan tiga hal, tidak ada yang lain.
//   (1) satu tanda tangan ECDSA P-256 atas digest konstan 32 B sesuai STANDAR: nonce
//       deterministik RFC 6979 (HMAC-SHA-256), sehingga r||s harus sama persis dengan
//       referensi independen di PC (tools/k1_check.py);
//   (2) latensinya terukur akurat dengan DWT CYCCNT pada 48 MHz;
//   (3) baseline memakai optimasi upstream Mbed TLS (include/mbedtls_k1_config.h), bukan heavy.
// Belum ada: RAM (tahap 3), WFI/%S_cpu (4), UART2 (5), jendela 60 s (6).
//
// Perintah (USB CDC 115200, akhiri newline):
//   I        info sistem (clock, profil Mbed TLS, DWT)
//   R[n]     n kali sign (default 30, maks 100), cetak CSV: i,cyc_sign,ms_sign,r||s(hex)
//
// Latensi = selisih CYCCNT bilangan bulat tak bertanda (aman terhadap satu kali limpahan).
// Pencetakan dan RNG seed berada DI LUAR jendela ukur.
// Build: pio run -e k1_sw   |  Upload: pio run -e k1_sw -t upload (cabut ST-Link setelah upload)
// ============================================================================
#include <Arduino.h>
#include "mbedtls/build_info.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"

#define ser SerialUSB

#define N_DEFAULT 30            // run per kelompok (Bab 3)
#define N_MAX     100
#define CMD_MAX   32

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

// Kunci publik (X||Y) = PRIVATE_KEY x G, dihitung offline. Hanya dicetak oleh `I`
// agar PC bisa memverifikasi; firmware tahap 1 tidak memverifikasi sendiri.
static const uint8_t PUBLIC_KEY_XY[64] = {
  0x96, 0xE6, 0xD7, 0xDD, 0xC5, 0xE8, 0xEF, 0xA4, 0x11, 0x7E, 0x63, 0xBD, 0xDE, 0x6C, 0x28, 0x52,
  0x62, 0xF2, 0x58, 0x5C, 0xE9, 0x6B, 0xFC, 0x9C, 0x58, 0x57, 0xF9, 0xBC, 0x5B, 0x2D, 0xEA, 0xF7,
  0x29, 0x4D, 0x5C, 0xB0, 0x52, 0x19, 0x50, 0x74, 0x0B, 0x76, 0xE9, 0x8B, 0x7A, 0x91, 0x45, 0xB1,
  0x80, 0x3F, 0x97, 0xD1, 0x63, 0xC2, 0x88, 0x8F, 0x64, 0xA6, 0xBE, 0x0E, 0xB1, 0xBB, 0xAD, 0x99
};

// ---------------------------------------------------------------------------
// DWT CYCCNT
static bool cyccnt_start() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  uint32_t a = DWT->CYCCNT;
  __NOP(); __NOP(); __NOP(); __NOP();
  return DWT->CYCCNT != a;
}

static inline uint32_t cycles() { return DWT->CYCCNT; }

// ---------------------------------------------------------------------------
// RNG HANYA untuk blinding (countermeasure side-channel; Mbed TLS mewajibkannya tidak NULL).
// Nonce k BUKAN dari sini: k deterministik per RFC 6979, jadi r||s tidak bergantung pada RNG ini.
// Benih dari derau ADC PA0 saat boot, diperluas xorshift128. Tidak aman kriptografis,
// cukup untuk blinding pada benchmark.
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
  return 0;                                     // konvensi Mbed TLS: 0 = sukses
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
// Kripto: grup P-256 dan kunci privat dimuat sekali saat boot.
static mbedtls_ecp_group g_grp;
static mbedtls_mpi       g_d;
static bool              g_ready = false;

static bool crypto_init() {
  mbedtls_ecp_group_init(&g_grp);
  mbedtls_mpi_init(&g_d);
  return mbedtls_ecp_group_load(&g_grp, MBEDTLS_ECP_DP_SECP256R1) == 0
      && mbedtls_mpi_read_binary(&g_d, PRIVATE_KEY, 32) == 0;
}

// Satu operasi "sign": digest 32 B -> r||s (64 B). Hanya fungsi ini yang diukur.
// noinline agar kompiler tidak menggeser kode ke luar kurung CYCCNT.
static int __attribute__((noinline)) sign_digest(const uint8_t *digest, uint8_t *sig) {
  mbedtls_mpi r, s;
  mbedtls_mpi_init(&r); mbedtls_mpi_init(&s);
  // Digest dipakai apa adanya sebagai h1 RFC 6979 (SHA-256 untuk HMAC-DRBG nonce).
  int ret = mbedtls_ecdsa_sign_det_ext(&g_grp, &r, &s, &g_d, digest, 32,
                                       MBEDTLS_MD_SHA256, rng_cb, NULL);
  if (ret == 0) ret = mbedtls_mpi_write_binary(&r, sig, 32);
  if (ret == 0) ret = mbedtls_mpi_write_binary(&s, sig + 32, 32);
  mbedtls_mpi_free(&r); mbedtls_mpi_free(&s);
  return ret;
}

// ---------------------------------------------------------------------------
// Cetak (tanpa floating point)
static void print_hex(const uint8_t *p, size_t n) {
  static const char H[] = "0123456789ABCDEF";
  for (size_t i = 0; i < n; i++) { ser.write(H[p[i] >> 4]); ser.write(H[p[i] & 0xF]); }
}

// siklus -> "xxx.yyy" ms (mikro-detik dibulatkan ke bawah), 64-bit agar tidak meluap
static void print_ms(uint32_t cyc) {
  uint64_t us = (uint64_t)cyc * 1000000ULL / SystemCoreClock;
  ser.printf("%lu.%03lu", (unsigned long)(us / 1000), (unsigned long)(us % 1000));
}

static uint32_t parse_u32(const char *p, uint32_t def) {
  if (*p < '0' || *p > '9') return def;
  uint32_t v = 0;
  while (*p >= '0' && *p <= '9') v = v * 10 + (uint32_t)(*p++ - '0');
  return v;
}

static void cmd_info() {
  ser.printf("# MCU clock : %lu Hz\n", (unsigned long)SystemCoreClock);
  ser.printf("# Library   : Mbed TLS %s, ECDSA secp256r1\n", MBEDTLS_VERSION_STRING);
#if defined(MBEDTLS_HAVE_ASM) && defined(MBEDTLS_ECP_NIST_OPTIM) && defined(MBEDTLS_ECDSA_DETERMINISTIC)
  ser.printf("# Profil    : optimasi upstream (ASM=1 NIST_OPTIM=1 comb=%d window=%d)\n",
             (int)MBEDTLS_ECP_FIXED_POINT_OPTIM, (int)MBEDTLS_ECP_WINDOW_SIZE);
  ser.println("# Nonce     : deterministik RFC 6979 (HMAC-SHA-256)");
#else
  ser.println("# Profil    : TIDAK STANDAR - cek -DMBEDTLS_CONFIG_FILE");
#endif
  ser.print ("# Digest    : "); print_hex(DIGEST, 32); ser.println();
  ser.print ("# PubKey XY : "); print_hex(PUBLIC_KEY_XY, 64); ser.println();
}

// n kali sign. Urutan per iterasi: ukur -> (di luar ukur) cetak.
static void cmd_run(uint32_t n) {
  uint8_t sig[64];
  uint64_t sum = 0;
  uint32_t mn = 0xFFFFFFFF, mx = 0, fail = 0;

  ser.println("i,cyc_sign,ms_sign,rs_hex");
  for (uint32_t i = 1; i <= n; i++) {
    uint32_t t0 = cycles();
    int ret = sign_digest(DIGEST, sig);
    uint32_t cyc = cycles() - t0;               // selisih tak bertanda: aman untuk 1 limpahan

    if (ret != 0) { fail++; ser.printf("%lu,ERR,%d\n", (unsigned long)i, ret); continue; }
    sum += cyc;
    if (cyc < mn) mn = cyc;
    if (cyc > mx) mx = cyc;
    ser.printf("%lu,%lu,", (unsigned long)i, (unsigned long)cyc);
    print_ms(cyc); ser.write(','); print_hex(sig, 64); ser.println();
  }

  uint32_t ok = n - fail;
  ser.printf("# n=%lu ok=%lu gagal=%lu\n", (unsigned long)n, (unsigned long)ok, (unsigned long)fail);
  if (ok) {
    ser.print("# mean="); print_ms((uint32_t)(sum / ok));
    ser.print(" ms  min="); print_ms(mn);
    ser.print(" ms  max="); print_ms(mx);
    ser.printf(" ms  (spread=%lu siklus)\n", (unsigned long)(mx - mn));
  }
  ser.println("# SELESAI");
}

// ---------------------------------------------------------------------------
void setup() {
  ser.begin(115200);
  bool dwt_ok = cyccnt_start();
  rng_seed();
  g_ready = crypto_init();

  for (uint32_t t = millis(); !ser && (millis() - t) < 10000; ) { }
  ser.println("K1 SOFTWARE tahap 1 (base standar, RFC 6979) siap. Perintah: I | R[n]");
  if (!dwt_ok)  ser.println("# PERINGATAN: DWT CYCCNT tidak berjalan");
  if (!g_ready) ser.println("# PERINGATAN: gagal memuat grup P-256 / kunci");
}

void loop() {
  static char line[CMD_MAX];
  static int n = 0;
  if (ser.available() <= 0) return;
  char c = ser.read();
  if (c == '\r') return;
  if (c != '\n') { if (n < CMD_MAX - 1) line[n++] = c; return; }
  line[n] = '\0'; n = 0;

  if (line[0] && !g_ready) { ser.println("-1"); return; }
  switch (line[0]) {
    case 'I': cmd_info(); break;
    case 'R': {
      uint32_t k = parse_u32(line + 1, N_DEFAULT);
      if (k < 1 || k > N_MAX) { ser.println("-1"); break; }
      cmd_run(k);
      break;
    }
    case '\0': break;
    default: ser.println("-1");
  }
}
