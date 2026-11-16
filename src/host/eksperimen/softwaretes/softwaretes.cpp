// ============================================================================
// Uji ECDSA P-256 murni perangkat lunak di STM32F103 (kelompok K1: Software-Based)
// Library : micro-ecc (kurva secp256r1 / NIST P-256), rweather/Crypto (SHA-256)
// Metrik  : latensi (siklus DWT->CYCCNT) dan RAM (statik, heap, puncak stack per operasi),
//           diukur dengan alat yang sama dengan firmware utama (include/bench.h).
// Clock   : bawaan STM32duino generic F103C8 + USB = 48 MHz (sama dengan main.cpp).
// Build   : pio run -e softwaretes -t upload
//
// Perintah serial (115200, akhiri dengan newline):
//   B[n]          -> benchmark n kali (default 30) dengan digest konstan (tanpa hash)
//   B[n]|<pesan>  -> benchmark n kali: SHA-256(pesan) di host lalu tanda tangan
//   M<pesan>      -> sekali: SHA-256(pesan) + tanda tangan, cetak digest|r||s|ukuran
//   S<hex64>      -> tandatangani digest 32 byte yang diberikan, cetak r||s|ukuran
//   P             -> cetak kunci publik (X||Y) dan info sistem
// ============================================================================
#include <Arduino.h>
#include <uECC.h>
#include "bench.h"

#define ser SerialUSB

#define N_DEFAULT  30      // jumlah sampel per skenario (Bab 3.5.2)
#define N_MAX      1000
#define LINE_MAX   255

// ---------------------------------------------------------------------------
// Data uji konstan. Digest = SHA-256("Skripsi ECDSA P-256 software vs DS28E38")
static const uint8_t DIGEST[32] = {
  0xD7, 0x2C, 0x10, 0x39, 0x78, 0x44, 0x3C, 0x67, 0xEA, 0xD7, 0x54, 0x29, 0x10, 0xF0, 0xEB, 0xF1,
  0xC8, 0xBA, 0xF6, 0x31, 0x82, 0xC0, 0x7E, 0x06, 0x2B, 0xCC, 0x6F, 0xC2, 0xE9, 0xA8, 0xA5, 0x0E
};

// Kunci privat UJI, disimpan statis di Flash. Ini memang model ancaman skema
// software-based (kunci terekspos di memori MCU). JANGAN dipakai untuk produksi.
static const uint8_t PRIVATE_KEY[32] = {
  0x64, 0x3E, 0xC9, 0x08, 0xDC, 0x6A, 0xF9, 0x93, 0xA3, 0x28, 0x79, 0x13, 0x13, 0x24, 0x1E, 0x69,
  0x3F, 0xAA, 0xD7, 0xA2, 0x33, 0x42, 0x08, 0xF1, 0x2C, 0x99, 0x02, 0x1A, 0xD3, 0xE0, 0x22, 0x3D
};

static uint8_t g_public_key[64];
static uECC_Curve g_curve;

// ---------------------------------------------------------------------------
// RNG untuk nonce k ECDSA. STM32F103 tidak punya TRNG: benih dari derau ADC saat boot,
// diperluas dengan xorshift128. Cepat (tidak mengganggu latensi) tetapi TIDAK aman
// secara kriptografis; cukup untuk benchmark.
static uint32_t rs[4];

static uint32_t xorshift128() {
  uint32_t t = rs[3], s = rs[0];
  rs[3] = rs[2]; rs[2] = rs[1]; rs[1] = s;
  t ^= t << 11; t ^= t >> 8;
  return rs[0] = t ^ s ^ (s >> 19);
}

static int rng(uint8_t *dest, unsigned size) {
  while (size) {
    uint32_t r = xorshift128();
    for (int i = 0; i < 4 && size; i++, size--) { *dest++ = (uint8_t)r; r >>= 8; }
  }
  return 1;
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
// Utilitas cetak (printf float tidak aktif di newlib-nano, jadi pakai fixed-point)
static void print_hex(const uint8_t *p, size_t n) {
  static const char H[] = "0123456789ABCDEF";
  for (size_t i = 0; i < n; i++) { ser.write(H[p[i] >> 4]); ser.write(H[p[i] & 0xF]); }
}

static void print_fixed(double v, int dec) {
  if (v < 0) { ser.write('-'); v = -v; }
  uint32_t scale = 1;
  for (int i = 0; i < dec; i++) scale *= 10;
  uint64_t x = (uint64_t)(v * scale + 0.5);
  ser.print((uint32_t)(x / scale));
  if (dec) {
    char buf[12];
    snprintf(buf, sizeof(buf), ".%0*lu", dec, (unsigned long)(x % scale));
    ser.print(buf);
  }
}

static double cyc_to_ms(uint32_t c) { return (double)c * 1000.0 / (double)SystemCoreClock; }

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

// ---------------------------------------------------------------------------
// Satu operasi terukur: [SHA-256(pesan)] + uECC_sign
typedef struct {
  uint32_t cyc_hash;    // siklus SHA-256 pesan (0 jika digest diberikan langsung)
  uint32_t cyc_sign;    // siklus uECC_sign
  uint32_t stack;       // byte stack terdalam yang dipakai hash + sign
  int32_t  heap_delta;  // perubahan heap (micro-ecc dan SHA256 tidak malloc)
  bool     ok;
} OpSample;

// msg != NULL: digest dihitung dari pesan ke `digest`; msg == NULL: `digest` dipakai apa adanya
static OpSample __attribute__((noinline))
measure_op(const char *msg, size_t len, uint8_t *digest, uint8_t *sig) {
  OpSample s = {0, 0, 0, 0, false};
  uint32_t heap0 = (uint32_t)sbrk(0);
  stack_paint();
  uint32_t sp0 = get_sp();

  uint32_t t0 = cycles();
  if (msg) sha256_msg(msg, len, digest);
  uint32_t t1 = cycles();
  int r = uECC_sign(PRIVATE_KEY, digest, 32, sig, g_curve);
  uint32_t t2 = cycles();

  s.cyc_hash   = msg ? t1 - t0 : 0;
  s.cyc_sign   = t2 - t1;
  s.stack      = sp0 - stack_low_water();
  s.heap_delta = (int32_t)((uint32_t)sbrk(0) - heap0);
  s.ok         = (r == 1);
  return s;
}

static void print_sysinfo() {
  ser.printf("# MCU clock     : %lu Hz\n", (unsigned long)SystemCoreClock);
  ser.println("# Library       : micro-ecc (secp256r1/NIST P-256) + rweather/Crypto SHA-256");
  ser.printf("# RAM total     : %lu B\n", (unsigned long)((uint32_t)&_estack - (uint32_t)&_sdata));
  ser.printf("# RAM statik    : %lu B (.data+.bss seluruh firmware)\n", (unsigned long)ram_static_bytes());
  ser.printf("# Heap terpakai : %lu B\n", (unsigned long)heap_bytes());
  ser.print("# Public key    : "); print_hex(g_public_key, 64); ser.println();
}

// ---------------------------------------------------------------------------
typedef struct { double mean, m2, mn, mx; } Stat;   // statistik berjalan (Welford)

static void stat_add(Stat &st, int i, double v) {
  double d = v - st.mean;
  st.mean += d / (i + 1);
  st.m2   += d * (v - st.mean);
  if (v < st.mn) st.mn = v;
  if (v > st.mx) st.mx = v;
}

static void stat_print(const char *label, const Stat &st, int n) {
  ser.printf("# %-10s ms: mean=", label); print_fixed(st.mean, 3);
  ser.print(" sd="); print_fixed(n > 1 ? sqrt(st.m2 / (n - 1)) : 0, 3);
  ser.print(" min="); print_fixed(st.mn, 3);
  ser.print(" max="); print_fixed(st.mx, 3); ser.println();
}

static void run_benchmark(int n, const char *msg) {
  Stat st_hash = {0, 0, 1e30, 0}, st_sign = {0, 0, 1e30, 0}, st_total = {0, 0, 1e30, 0};
  uint32_t stack_min = 0xFFFFFFFF, stack_max = 0;
  int n_ok = 0, n_valid = 0;
  uint8_t digest[32], sig[64];
  size_t len = msg ? strlen(msg) : 0;
  if (!msg) memcpy(digest, DIGEST, 32);

  print_sysinfo();
  if (msg) ser.printf("# Pesan         : \"%s\" (%u B), di-hash SHA-256 di host\n", msg, (unsigned)len);
  else     ser.println("# Pesan         : (tidak ada) digest konstan, tanpa hash");
  ser.printf("# Benchmark N=%d (verifikasi tidak termasuk latensi)\n", n);
  ser.println("i,cyc_hash,cyc_sign,ms_hash,ms_sign,ms_total,stack_bytes,heap_delta,sign_ok,verify_ok");

  for (int i = 0; i < n; i++) {
    OpSample s = measure_op(msg, len, digest, sig);
    bool valid = s.ok && uECC_verify(g_public_key, digest, 32, sig, g_curve);
    double mh = cyc_to_ms(s.cyc_hash), ms = cyc_to_ms(s.cyc_sign);
    stat_add(st_hash, i, mh); stat_add(st_sign, i, ms); stat_add(st_total, i, mh + ms);
    if (s.stack < stack_min) stack_min = s.stack;
    if (s.stack > stack_max) stack_max = s.stack;
    n_ok += s.ok; n_valid += valid;

    ser.printf("%d,%lu,%lu,", i + 1, (unsigned long)s.cyc_hash, (unsigned long)s.cyc_sign);
    print_fixed(mh, 3); ser.write(','); print_fixed(ms, 3); ser.write(','); print_fixed(mh + ms, 3);
    ser.printf(",%lu,%ld,%d,%d\n", (unsigned long)s.stack, (long)s.heap_delta, s.ok, valid);
  }

  ser.print("# Digest        : "); print_hex(digest, 32); ser.println();
  stat_print("T_hash", st_hash, n);
  stat_print("T_sign", st_sign, n);
  stat_print("T_total", st_total, n);
  ser.printf("# Stack operasi: min=%lu max=%lu B\n", (unsigned long)stack_min, (unsigned long)stack_max);
  ser.printf("# Valid        : sign %d/%d, verify %d/%d\n", n_ok, n, n_valid, n);
  ser.println("# SELESAI");
}

// Satu operasi: cetak [digest|] r||s | cyc_hash,cyc_sign,ms_hash,ms_sign,ms_total,stack
static void sign_once(const char *msg, uint8_t *digest) {
  uint8_t sig[64];
  OpSample s = measure_op(msg, msg ? strlen(msg) : 0, digest, sig);
  if (!s.ok) { ser.println("-1"); return; }
  if (msg) { print_hex(digest, 32); ser.write('|'); }
  print_hex(sig, 64);
  double mh = cyc_to_ms(s.cyc_hash), ms = cyc_to_ms(s.cyc_sign);
  ser.printf("|%lu,%lu,", (unsigned long)s.cyc_hash, (unsigned long)s.cyc_sign);
  print_fixed(mh, 3); ser.write(','); print_fixed(ms, 3); ser.write(','); print_fixed(mh + ms, 3);
  ser.printf(",%lu\n", (unsigned long)s.stack);
}

// ---------------------------------------------------------------------------
void setup() {
  ser.begin(115200);
  bool dwt_ok = cyccnt_init();
  rng_seed();
  uECC_set_rng(rng);
  g_curve = uECC_secp256r1();
  bool key_ok = uECC_compute_public_key(PRIVATE_KEY, g_public_key, g_curve);

  for (uint32_t t = millis(); !ser && (millis() - t) < 10000; ) { }
  ser.println("SOFTWARETES ECDSA P-256 siap. Perintah: B[n][|pesan] | M<pesan> | S<hex64> | P");
  if (!dwt_ok) ser.println("# PERINGATAN: DWT CYCCNT tidak berjalan");
  if (!key_ok) ser.println("# PERINGATAN: gagal menghitung kunci publik");
}

void loop() {
  static char line[LINE_MAX];
  static int n = 0;
  if (ser.available() <= 0) return;
  char c = ser.read();
  if (c == '\r') return;
  if (c != '\n') { if (n < LINE_MAX - 1) line[n++] = c; return; }
  line[n] = '\0'; n = 0;

  switch (line[0]) {
    case 'B': {
      char *msg = strchr(line, '|');
      if (msg) *msg++ = '\0';
      int k = line[1] ? atoi(line + 1) : N_DEFAULT;
      if (k < 1 || k > N_MAX) { ser.println("-1"); break; }
      run_benchmark(k, msg);
      break;
    }
    case 'M': { uint8_t d[32]; sign_once(line + 1, d); break; }
    case 'S': {
      uint8_t d[32];
      if (!parse_hex(line + 1, d, 32)) { ser.println("-1"); break; }
      sign_once(nullptr, d);
      break;
    }
    case 'P': print_sysinfo(); break;
    case '\0': break;
    default: ser.println("-1");
  }
}
