// ============================================================================
// K1 software (Mbed TLS, ECDSA P-256) — TAHAP 1b: BASE + HARDENING (keputusan 13 Nov)
//
// Aturan "build from base": tahap ini HANYA membuktikan hal berikut, tidak ada yang lain.
//   (1) satu tanda tangan ECDSA P-256 atas digest konstan 32 B, valid di PC (tools/k1_check.py);
//   (2) latensinya terukur akurat dengan DWT CYCCNT pada 48 MHz;
//   (3) optimasi upstream Mbed TLS tetap aktif (include/mbedtls_k1_config.h), bukan heavy.
// Hardening yang ditambahkan di atas tahap 1 (tahap 1 lama: RFC 6979 murni, 276,436 ms):
//   H1. Nonce HEDGED: HMAC-DRBG di-seed (entropi 32 B || kunci privat d || digest), lalu dipakai
//       untuk nonce k DAN blinding. Kalau entropi ternyata lemah, nonce paling buruk setara
//       RFC 6979 (karena d||digest ikut di seed). Konsekuensi: r||s berbeda tiap sign, tidak
//       lagi bisa dibandingkan byte-per-byte dengan referensi RFC 6979.
//   H2. Entropi: derau ADC PA0 + CYCCNT + micros() + UID chip + penghitung, dicampur SHA-256.
//       BUKAN TRNG (F103 tidak punya); jangan diklaim sebagai sumber acak kriptografis penuh.
//   H3. Zeroize: d hanya dimuat ke RAM selama sign (g_d global dihapus); seed, DRBG, mpi r/s/d
//       dihapus (mbedtls_platform_zeroize / *_free) segera setelah dipakai.
//   H4. Konfigurasi ketat: MBEDTLS_CHECK_RETURN_WARNING (hasil fungsi tak boleh diabaikan).
// Jendela ukur = muat d + seed DRBG + sign + zeroize. Pengumpulan entropi (ADC) DI LUAR jendela
// (sama seperti RNG seed sebelumnya); catat saat membandingkan dengan DS28E38 (TRNG internal).
// TAHAP 2 (14 Nov): ukur RAM puncak operasi sign pada konfigurasi 1c (comb mati, lihat
//   include/mbedtls_k1_config.h). Satu fungsi baru: perintah M. Jalur sign TIDAK diubah.
//   RAM puncak = statik (.data+.bss) + heap puncak + stack puncak.
//   - heap: newlib tidak pernah mengembalikan memori ke sistem, jadi (sbrk(0) - _end) setelah
//     operasi = jejak heap puncak (termasuk overhead header malloc), bukan hanya byte yang diminta.
//   - stack: area bebas di atas heap dicat pola 0xA5A5A5A5 SESUDAH satu sign pemanasan (heap sudah
//     di puncak), lalu satu operasi penuh (entropi + sign) dijalankan; alamat terendah yang
//     tertimpa = stack puncak. Termasuk jejak ISR (USB) bila kebetulan menyela.
// TAHAP 3 (14 Nov): + perintah W = jendela pengamatan dengan WFI, pewaktu hardware, pin PB1, %S_cpu
//   (permintaan SINTETIS dari TIM4; lihat blok "Tahap 3" di bawah). Jalur sign tidak diubah.
// TAHAP 4 (14 Nov): + perintah A = jendela dengan permintaan NYATA dari attacker lewat USART2
//   (DMA, satu bangun per frame; lihat blok "Tahap 4"); setup() tidak lagi menunggu USB.
// Belum ada: handshake "papan siap" dan RUN_START/RUN_END (tahap 5).
//
// Perintah (USB CDC 115200, akhiri newline):
//   I        info sistem (clock, profil Mbed TLS, DWT)
//   R[n]     n kali sign (default 30, maks 100), cetak CSV: i,cyc_sign,ms_sign,r||s(hex)
//   M[n]     ukur RAM puncak: n pengukuran (default 10, maks 100), cetak CSV + ringkasan
//   W[s][,p] jendela s detik (default 10, maks 120), permintaan sintetis tiap p ms (default 1000,
//            100..6000); laporan T_obs, T_sleep, %S_cpu, dilayani/terlewat. Cetak setelah jendela.
//
// Latensi = selisih CYCCNT bilangan bulat tak bertanda (aman terhadap satu kali limpahan).
// Pencetakan dan RNG seed berada DI LUAR jendela ukur.
// Build: pio run -e k1_sw   |  Upload: pio run -e k1_sw -t upload (cabut ST-Link setelah upload)
// ============================================================================
#include <Arduino.h>
#include "mbedtls/build_info.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h"

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
static mbedtls_ecp_group g_grp;
static bool              g_ready = false;

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
  // Profil dilaporkan APA ADANYA (bukan hanya "standar" atau tidak): dipakai eksperimen
  // pengaruh per opsi (ASM / NIST_OPTIM) dan pemeriksa PC.
#if defined(MBEDTLS_HAVE_ASM)
  const int f_asm = 1;
#else
  const int f_asm = 0;
#endif
#if defined(MBEDTLS_ECP_NIST_OPTIM)
  const int f_nist = 1;
#else
  const int f_nist = 0;
#endif
  ser.printf("# Profil    : ASM=%d NIST_OPTIM=%d comb=%d window=%d\n", f_asm, f_nist,
             (int)MBEDTLS_ECP_FIXED_POINT_OPTIM, (int)MBEDTLS_ECP_WINDOW_SIZE);
  ser.println("# Nonce     : hedged (HMAC-DRBG seed: entropi||d||digest), bukan RFC 6979 murni");
#if defined(MBEDTLS_CHECK_RETURN_WARNING)
  ser.println("# Hardening : blinding via DRBG, zeroize d/seed/DRBG, CHECK_RETURN_WARNING");
#else
  ser.println("# Hardening : blinding via DRBG, zeroize d/seed/DRBG");
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
    uint8_t ent[32];
    int ret = harvest_entropy(ent);             // di luar jendela ukur
    uint32_t t0 = cycles();
    if (ret == 0) ret = sign_digest(DIGEST, ent, sig);
    uint32_t cyc = cycles() - t0;               // selisih tak bertanda: aman untuk 1 limpahan
    mbedtls_platform_zeroize(ent, sizeof ent);

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
// Tahap 2: RAM puncak. Simbol linker: awal RAM, akhir .bss, puncak stack.
extern "C" char *sbrk(int i);
extern char _sdata, _end, _estack;

#define STACK_PAINT 0xA5A5A5A5UL
#define HEAP_MARGIN 256                        // jarak aman di atas heap saat mengecat stack

static inline uint32_t get_sp() {
  uint32_t sp;
  __asm__ volatile ("mov %0, sp" : "=r" (sp));
  return sp;
}

// Cat area bebas [sbrk(0)+margin, SP-16). noinline agar SP yang dibaca = SP fungsi ini
// (frame pemanggil di atasnya tidak ikut tertimpa). Mengembalikan batas bawah cat.
static volatile uint32_t *__attribute__((noinline)) stack_paint() {
  volatile uint32_t *lo = (volatile uint32_t *)(((uint32_t)sbrk(0) + HEAP_MARGIN + 3) & ~3UL);
  volatile uint32_t *hi = (volatile uint32_t *)((get_sp() - 16) & ~3UL);
  if (lo >= hi) return nullptr;
  for (volatile uint32_t *p = lo; p < hi; p++) *p = STACK_PAINT;
  return lo;
}

// Alamat terendah yang sudah tertimpa (awal stack puncak).
static uint32_t stack_low_water(volatile uint32_t *lo) {
  volatile uint32_t *p = lo;
  while (*p == STACK_PAINT) p++;
  return (uint32_t)p;
}

// Satu operasi penuh yang sama dengan R: entropi + sign. Hasil r||s dibuang.
static int __attribute__((noinline)) one_operation() {
  uint8_t ent[32], sig[64];
  int ret = harvest_entropy(ent);
  if (ret == 0) ret = sign_digest(DIGEST, ent, sig);
  mbedtls_platform_zeroize(ent, sizeof ent);
  return ret;
}

static void cmd_ram(uint32_t n) {
  const uint32_t total  = (uint32_t)&_estack - (uint32_t)&_sdata;    // seluruh RAM
  const uint32_t statik = (uint32_t)&_end - (uint32_t)&_sdata;       // .data + .bss
  const uint32_t heap0  = (uint32_t)sbrk(0) - (uint32_t)&_end;       // heap setelah boot (grup P-256)

  // Pemanasan: satu operasi penuh supaya heap mencapai puncaknya sebelum stack dicat.
  if (one_operation() != 0) { ser.println("# GAGAL: operasi pemanasan"); return; }
  const uint32_t heap_warm = (uint32_t)sbrk(0) - (uint32_t)&_end;

  ser.println("i,heap,stack_puncak,statik,pakai_total,sisa");
  uint32_t heap_max = 0, stack_max = 0, fail = 0, heap_naik = 0;
  for (uint32_t i = 1; i <= n; i++) {
    volatile uint32_t *lo = stack_paint();
    if (!lo) { ser.println("# GAGAL: area cat stack kosong"); return; }
    const uint32_t brk0 = (uint32_t)sbrk(0);
    int ret = one_operation();
    const uint32_t brk1 = (uint32_t)sbrk(0);
    const uint32_t stack = (uint32_t)&_estack - stack_low_water(lo);
    const uint32_t heap  = brk1 - (uint32_t)&_end;
    if (ret != 0) { fail++; ser.printf("%lu,ERR,%d\n", (unsigned long)i, ret); continue; }
    if (brk1 != brk0) heap_naik++;              // heap tumbuh = cat stack tak sah untuk iterasi ini
    if (heap > heap_max)   heap_max = heap;
    if (stack > stack_max) stack_max = stack;
    const uint32_t pakai = statik + heap + stack;
    ser.printf("%lu,%lu,%lu,%lu,%lu,%lu\n", (unsigned long)i, (unsigned long)heap,
               (unsigned long)stack, (unsigned long)statik, (unsigned long)pakai,
               (unsigned long)(total - pakai));
  }
  const uint32_t pakai_max = statik + heap_max + stack_max;
  ser.printf("# RAM total=%lu statik=%lu heap_boot=%lu heap_pemanasan=%lu\n", (unsigned long)total,
             (unsigned long)statik, (unsigned long)heap0, (unsigned long)heap_warm);
  ser.printf("# PUNCAK heap=%lu stack=%lu statik=%lu -> pakai=%lu B (%lu.%lu%%) sisa=%lu B\n",
             (unsigned long)heap_max, (unsigned long)stack_max, (unsigned long)statik,
             (unsigned long)pakai_max, (unsigned long)(pakai_max * 100 / total),
             (unsigned long)(pakai_max * 1000 / total % 10), (unsigned long)(total - pakai_max));
  ser.printf("# n=%lu gagal=%lu heap_tumbuh_saat_ukur=%lu\n", (unsigned long)n, (unsigned long)fail,
             (unsigned long)heap_naik);
  ser.println("# SELESAI");
}

// ---------------------------------------------------------------------------
// Tahap 3: WFI + pewaktu hardware + pin status PB1 + %S_cpu.
// Fungsi tahap ini satu: satu JENDELA PENGAMATAN pendek di mana CPU tidur (WFI) di antara
// "permintaan" sintetis dari pewaktu hardware TIM4, dan T_sleep diakumulasi dari pewaktu yang
// TETAP BERJALAN saat tidur. Permintaan nyata lewat UART2 baru di tahap 4.
//   - Waktu hardware 32-bit, resolusi 1 us: TIM2 (16-bit, PSC -> 1 MHz) -> TRGO update -> TIM3
//     (mode clock eksternal 1, ITR1) = 16 bit atas. Tanpa interupsi, tak membangunkan CPU.
//   - Permintaan sintetis: TIM4 (HardwareTimer) interupsi tiap P ms. Tik ke-K mengakhiri jendela
//     (bukan permintaan). Permintaan yang tiba saat sudah ada yang menunggu menyatu (buffer satu
//     frame): arrived - served = yang terlewat.
//   - Kebijakan tidur: interupsi dimatikan (PRIMASK) -> cek flag -> SysTick DIMATIKAN -> WFI;
//     WFI tetap bangun oleh interupsi yang pending walau PRIMASK=1, jadi tak ada race. Setelah
//     bangun SysTick dinyalakan lagi dan uwTick dikoreksi sebesar waktu tidur (agar timeout HAL,
//     mis. analogRead, tetap benar).
//   - Pin status PB1: 1 saat CPU aktif, 0 saat akan WFI.
//   - %S_cpu = T_sleep / T_obs. T_obs = dari mulai jendela sampai keluar loop (bila sign sedang
//     berjalan saat tik terakhir, selesai dulu). Pemeriksa konsistensi internal: T_obs - T_sleep
//     harus = total segmen aktif yang diukur CYCCNT (jam inti), diukur lepas dari pewaktu.
//   - Pembanding independen (duty pin PB1 di logger) belum ada; pembacaan balik IDR PB1 dilaporkan.
#define PIN_STATUS_HI() (GPIOB->BSRR = GPIO_BSRR_BS1)
#define PIN_STATUS_LO() (GPIOB->BSRR = GPIO_BSRR_BR1)

static bool hw_time_init() {
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
  const uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  const uint32_t tclk  = (RCC->CFGR & RCC_CFGR_PPRE1) ? 2 * pclk1 : pclk1;   // jam pewaktu APB1
  if (tclk == 0 || (tclk % 1000000UL) != 0) return false;
  TIM2->CR1 = 0; TIM3->CR1 = 0;
  TIM2->PSC = tclk / 1000000UL - 1;
  TIM2->ARR = 0xFFFF;
  TIM2->CR2 = TIM_CR2_MMS_1;                    // TRGO = update
  TIM2->EGR = TIM_EGR_UG;                       // muat PSC (TIM3 belum jadi budak)
  TIM3->PSC = 0;
  TIM3->ARR = 0xFFFF;
  TIM3->SMCR = TIM_SMCR_TS_0;                   // trigger = ITR1 (TIM2 TRGO)
  TIM3->SMCR |= TIM_SMCR_SMS;                   // SMS=111: clock eksternal mode 1
  TIM2->CNT = 0; TIM3->CNT = 0;
  TIM3->CR1 = TIM_CR1_CEN;
  TIM2->CR1 = TIM_CR1_CEN;
  return true;
}

// Waktu hardware 32-bit (us). Ulangi bila bagian atas berubah di tengah baca, atau bila bagian
// bawah baru saja membungkus (TRGO ke TIM3 butuh beberapa siklus).
static inline uint32_t hw_us() {
  uint32_t hi, lo, hi2;
  do { hi = TIM3->CNT; lo = TIM2->CNT; hi2 = TIM3->CNT; } while (hi != hi2 || lo < 4);
  return (hi << 16) | lo;
}

static volatile uint32_t g_ticks, g_K, g_arrived, g_t_last, g_t_first, g_tick_min, g_tick_max;
static volatile bool     g_pending, g_end;

static void tick_isr() {
  const uint32_t t = hw_us();
  const uint32_t dt = t - g_t_last;
  g_t_last = t;
  if (g_ticks == 0) g_t_first = dt;
  else { if (dt < g_tick_min) g_tick_min = dt; if (dt > g_tick_max) g_tick_max = dt; }
  g_ticks++;
  if (g_ticks >= g_K) g_end = true;
  else { g_arrived++; g_pending = true; }
}

// ---------------------------------------------------------------------------
// Tahap 4: permintaan NYATA dari attacker lewat USART2 (PA3 = RX), menggantikan TIM4 sintetis
// (perintah A). Penerimaan: DMA1 kanal 6 mengisi satu frame 36 B; interupsi selesai-transfer (TC)
// terjadi SEKALI per frame, jadi satu frame = satu bangun CPU (paper). Deteksi idle-line lewat
// interupsi USART2 tak dipakai karena core Arduino sudah mendefinisikan USART2_IRQHandler
// (tabrakan simbol); gantinya: bila frame tak valid (sinkron geser, byte hilang), ISR membuang
// byte sampai flag IDLE USART terangkat (atau 10 ms) lalu memasang ulang DMA, sehingga
// perataan frame pulih pada jeda antar-frame (attacker >= 100 ms antar frame).
// Frame (src/attacker/attacker.cpp): [0]=0xA5 [1..2]=seq uint16 LE [3..34]=isi [35]=XOR [0..34].
// Isi frame TIDAK dipakai: yang ditandatangani tetap DIGEST konstan (variabel kontrol K1/K2).
// Jendela berakhir oleh SATU interupsi TIM4 satu-tembakan (bukan tik tiap detik, agar tak ada
// bangun tambahan). Permintaan sah yang tiba saat sudah ada yang menunggu menyatu (buffer satu
// frame): terlewat = datang - dilayani; hilang = selisih seq (frame yang tak pernah sampai).
#define RX_FRAME_LEN 36
#define RX_SOF       0xA5
#define RX_BAUD      115200UL

static uint8_t g_rx[RX_FRAME_LEN];
static volatile uint32_t g_rx_ok, g_rx_bad, g_rx_lost, g_rx_first_seq, g_rx_last_seq;
static volatile bool     g_rx_have;
static uint8_t g_rx_badfirst[RX_FRAME_LEN];   // salinan frame bad pertama (diagnosis)
static volatile uint32_t g_rx_badfirst_wait;      // lama menunggu IDLE di ISR (us) pada bad pertama

static void rx_arm() {
  DMA1_Channel6->CCR &= ~DMA_CCR_EN;
  DMA1_Channel6->CNDTR = RX_FRAME_LEN;
  DMA1_Channel6->CCR |= DMA_CCR_EN;
}

static void rx_init() {                         // sekali di setup; penerimaan baru aktif di rx_start()
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  GPIOA->CRL = (GPIOA->CRL & ~(0xFUL << 12)) | (0x4UL << 12);   // PA3: input floating
  USART2->CR1 = 0; USART2->CR2 = 0;
  USART2->BRR = (HAL_RCC_GetPCLK1Freq() + RX_BAUD / 2) / RX_BAUD;
  USART2->CR3 = USART_CR3_DMAR;
  USART2->CR1 = USART_CR1_UE;                   // RE dinyalakan di rx_start()
  DMA1_Channel6->CCR = 0;
  DMA1_Channel6->CPAR = (uint32_t)&USART2->DR;
  DMA1_Channel6->CMAR = (uint32_t)g_rx;
  DMA1_Channel6->CCR = DMA_CCR_MINC | DMA_CCR_TCIE;
  NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
  NVIC_EnableIRQ(DMA1_Channel6_IRQn);
}

static void rx_start() {
  g_rx_ok = g_rx_bad = g_rx_lost = g_rx_first_seq = g_rx_last_seq = 0;
  g_rx_have = false;
  g_rx_badfirst_wait = 0;
  memset(g_rx_badfirst, 0, RX_FRAME_LEN);
  (void)USART2->SR; (void)USART2->DR;           // buang sisa
  DMA1->IFCR = DMA_IFCR_CGIF6;
  NVIC_ClearPendingIRQ(DMA1_Channel6_IRQn);
  USART2->CR1 |= USART_CR1_RE;
  rx_arm();
#ifdef K1_RX_TEST_MISALIGN
  // HANYA UJI: frame pertama diharapkan 35 B (bukan 36) -> sengaja salah rata, untuk membuktikan
  // pemulihan (bad=1 lalu semua frame berikutnya ok). Aktifkan: PLATFORMIO_BUILD_FLAGS=-DK1_RX_TEST_MISALIGN
  DMA1_Channel6->CCR &= ~DMA_CCR_EN;
  DMA1_Channel6->CNDTR = RX_FRAME_LEN - 1;
  DMA1_Channel6->CCR |= DMA_CCR_EN;
#endif
}

static void rx_stop() {
  DMA1_Channel6->CCR &= ~DMA_CCR_EN;
  USART2->CR1 &= ~USART_CR1_RE;
}

extern "C" void DMA1_Channel6_IRQHandler(void) {
  DMA1->IFCR = DMA_IFCR_CGIF6;
  uint8_t x = 0;
  for (int i = 0; i < RX_FRAME_LEN - 1; i++) x ^= g_rx[i];
  if (g_rx[0] == RX_SOF && x == g_rx[RX_FRAME_LEN - 1]) {
    const uint16_t seq = (uint16_t)g_rx[1] | ((uint16_t)g_rx[2] << 8);
    if (g_rx_have) g_rx_lost += (uint16_t)(seq - (uint16_t)(g_rx_last_seq + 1));
    else { g_rx_first_seq = seq; g_rx_have = true; }
    g_rx_last_seq = seq;
    g_rx_ok++;
    g_arrived++;
    g_pending = true;
  } else {
    if (g_rx_bad++ == 0) memcpy(g_rx_badfirst, g_rx, RX_FRAME_LEN);
    // PENTING: matikan permintaan DMA USART selama resync. Bila tidak, byte sisa yang tiba saat
    // kanal nonaktif membuat permintaan DMA 'terkunci'; saat kanal dipasang ulang DMA membaca DR
    // basi (byte checksum) sebagai byte pertama frame berikutnya dan semua frame jadi bad (terbukti
    // di uji K1_RX_TEST_MISALIGN, 14 Nov). DMAR dinyalakan lagi SESUDAH kanal dipasang ulang.
    USART2->CR3 &= ~USART_CR3_DMAR;
    (void)USART2->SR; (void)USART2->DR;         // hapus IDLE basi
    const uint32_t t = hw_us();
    while (!(USART2->SR & USART_SR_IDLE) && (uint32_t)(hw_us() - t) < 10000UL) {
      if (USART2->SR & USART_SR_RXNE) (void)USART2->DR;
    }
    if (g_rx_bad == 1) g_rx_badfirst_wait = (uint32_t)(hw_us() - t);
    (void)USART2->SR; (void)USART2->DR;
    rx_arm();
    USART2->CR3 |= USART_CR3_DMAR;
    return;
  }
  rx_arm();
}

static void end_isr() { g_end = true; }

static void cmd_window(uint32_t secs, uint32_t p_ms, bool uart = false) {
  uint32_t K = secs * 1000UL / p_ms;
  if (K < 1 || uart) K = 1;
  static HardwareTimer t4(TIM4);                // statis: tanpa alokasi heap
  t4.pause();
  if (uart) {
    t4.setOverflow(secs * 1000000UL, MICROSEC_FORMAT);   // satu tembakan = akhir jendela
    t4.setCount(0);
    t4.attachInterrupt(end_isr);
  } else {
    t4.setOverflow(p_ms * 1000UL, MICROSEC_FORMAT);
    t4.setCount(0);
    t4.attachInterrupt(tick_isr);
  }

  g_ticks = 0; g_K = K; g_arrived = 0; g_pending = false; g_end = false;
  g_t_first = 0; g_tick_min = 0xFFFFFFFFUL; g_tick_max = 0;
  uint32_t served = 0, wfi_n = 0, spurious = 0, pin_salah = 0;
  uint64_t t_sleep = 0, act_cyc = 0, uw_rem = 0;

  TIM4->SR = 0; NVIC_ClearPendingIRQ(TIM4_IRQn);   // buang update palsu akibat setOverflow
  // USB CDC (hanya saat tersambung PC) memicu interupsi SOF tiap 1 ms yang membangunkan WFI terus-
  // menerus. Di bangku ukur sebenarnya USB-C hanya daya (tanpa D+/D-), jadi interupsi itu tak ada.
  // Selama jendela SOF/ESOF di-mask agar uji lewat USB sama dengan kondisi ukur; dipulihkan setelahnya.
  const uint32_t usb_cntr = USB->CNTR;
  USB->CNTR = usb_cntr & ~(USB_CNTR_SOFM | USB_CNTR_ESOFM);
  const uint32_t t0 = hw_us(), c0 = cycles();
  g_t_last = t0;
  uint32_t seg = c0;
  PIN_STATUS_HI();
  if (uart) rx_start();
  t4.resume();

  for (;;) {
    __disable_irq();
    if (!g_pending && !g_end) {
      act_cyc += (uint32_t)(cycles() - seg);
      PIN_STATUS_LO();
      if (GPIOB->IDR & GPIO_IDR_IDR1) pin_salah++;   // baca balik: harus 0
      SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
      const uint32_t ts = hw_us();
      __DSB();
      __WFI();
      const uint32_t tw = hw_us();
      SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
      PIN_STATUS_HI();
      seg = cycles();
      const uint32_t dt = tw - ts;
      t_sleep += dt; wfi_n++;
      uw_rem += dt; uwTick += (uint32_t)(uw_rem / 1000); uw_rem %= 1000;
      __enable_irq();                           // ISR yang menunggu berjalan di sini
      __ISB();                                  // Cortex-M3 bisa menunda masuk ISR 1 instruksi setelah CPSIE
      if (!g_pending && !g_end) spurious++;
      continue;
    }
    __enable_irq();
    if (g_end) break;
    __disable_irq(); g_pending = false; __enable_irq();
    if (one_operation() == 0) served++;
  }
  act_cyc += (uint32_t)(cycles() - seg);
  const uint32_t t_end = hw_us();
  const uint32_t c_end = cycles();
  PIN_STATUS_LO();
  if (uart) rx_stop();
  t4.pause();
  t4.detachInterrupt();
  USB->CNTR = usb_cntr;

  const uint64_t t_obs = (uint32_t)(t_end - t0);
  const uint64_t act_us = act_cyc * 1000000ULL / SystemCoreClock;
  const uint64_t cyc_total_us = (uint64_t)(uint32_t)(c_end - c0) * 1000000ULL / SystemCoreClock;
  const int64_t resid = (int64_t)t_obs - (int64_t)t_sleep - (int64_t)act_us;
  const uint64_t sp = t_obs ? t_sleep * 10000ULL / t_obs : 0;      // persen x100

  if (uart) ser.printf("# WINDOW attacker (UART2 DMA) %lu s, akhir jendela oleh TIM4 satu-tembakan\n",
                       (unsigned long)secs);
  else      ser.printf("# WINDOW periode=%lu ms K=%lu (tik ke-K mengakhiri jendela)\n", (unsigned long)p_ms,
                       (unsigned long)K);
  ser.printf("# T_obs=%lu us  T_sleep=%lu us  aktif(CYCCNT)=%lu us  sisa=%ld us\n",
             (unsigned long)t_obs, (unsigned long)t_sleep, (unsigned long)act_us, (long)resid);
  ser.printf("# S_cpu=%lu.%02lu %%\n", (unsigned long)(sp / 100), (unsigned long)(sp % 100));
  ser.printf("# permintaan: datang=%lu dilayani=%lu terlewat=%lu\n", (unsigned long)g_arrived,
             (unsigned long)served, (unsigned long)(g_arrived - served));
  ser.printf("# tidur: WFI=%lu bangun_bukan_permintaan=%lu pin_PB1_salah_baca=%lu\n",
             (unsigned long)wfi_n, (unsigned long)spurious, (unsigned long)pin_salah);
  if (uart) {
    const uint32_t sent = g_rx_have ? (uint16_t)(g_rx_last_seq - g_rx_first_seq) + 1UL : 0;
    ser.printf("# frame: ok=%lu bad=%lu hilang_seq=%lu seq=%lu..%lu terkirim=%lu\n",
               (unsigned long)g_rx_ok, (unsigned long)g_rx_bad, (unsigned long)g_rx_lost,
               (unsigned long)g_rx_first_seq, (unsigned long)g_rx_last_seq, (unsigned long)sent);
    if (g_rx_bad) {
      ser.print("# bad pertama: "); print_hex(g_rx_badfirst, RX_FRAME_LEN);
      ser.printf(" (tunggu IDLE %lu us)\n", (unsigned long)g_rx_badfirst_wait);
    }
  } else {
    ser.printf("# tik: pertama=%lu us min=%lu us maks=%lu us\n", (unsigned long)g_t_first,
               (unsigned long)(g_tick_min == 0xFFFFFFFFUL ? 0 : g_tick_min), (unsigned long)g_tick_max);
  }
  ser.printf("# CYCCNT sepanjang jendela=%lu us (bandingkan T_obs: >0 selisih = CYCCNT berhenti saat WFI)\n",
             (unsigned long)cyc_total_us);
  ser.println("# SELESAI");
}

// ---------------------------------------------------------------------------
void setup() {
  ser.begin(115200);
  bool dwt_ok = cyccnt_start();
  g_ready = crypto_init();
  const bool hw_ok = hw_time_init();
  pinMode(PB1, OUTPUT);
  PIN_STATUS_LO();
  rx_init();

  // TIDAK menunggu koneksi USB (tahap 4): di bangku ukur USB-C hanya daya, dan menunggu sampai
  // 10 s merusak jendela 60 s. Banner hanya dicetak bila PC sudah terhubung saat boot.
  if (ser) {
    ser.println("K1 SOFTWARE tahap 4 (konfigurasi 1c: hedged nonce, zeroize, tanpa comb) siap. Perintah: I | R[n] | M[n] | W[s][,p] | A[s]");
    if (!dwt_ok)  ser.println("# PERINGATAN: DWT CYCCNT tidak berjalan");
    if (!g_ready) ser.println("# PERINGATAN: gagal memuat grup P-256");
    if (!hw_ok)   ser.println("# PERINGATAN: jam pewaktu APB1 bukan kelipatan 1 MHz");
  }
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
    case 'M': {
      uint32_t k = parse_u32(line + 1, 10);
      if (k < 1 || k > N_MAX) { ser.println("-1"); break; }
      cmd_ram(k);
      break;
    }
    case 'W': {
      // W[detik][,periode_ms]  default 10 s, 1000 ms. Contoh: W60 | W6,6000 (tanpa permintaan = idle)
      uint32_t s = parse_u32(line + 1, 10), p = 1000;
      const char *comma = strchr(line, ',');
      if (comma) p = parse_u32(comma + 1, 1000);
      if (s < 1 || s > 120 || p < 100 || p > 6000) { ser.println("-1"); break; }
      cmd_window(s, p);
      break;
    }
    case 'A': {
      // A[detik]  jendela dengan permintaan NYATA dari attacker (UART2), default 60 s, maks 80 s
      // (batas 16-bit TIM4 + prescaler: 89 s pada 48 MHz). Attacker harus sudah mengirim.
      uint32_t s = parse_u32(line + 1, 60);
      if (s < 1 || s > 80) { ser.println("-1"); break; }
      cmd_window(s, 1000, true);
      break;
    }
    case '\0': break;
    default: ser.println("-1");
  }
}
