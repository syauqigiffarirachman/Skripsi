// ============================================================================
// Konfigurasi Mbed TLS 3.6.7 untuk baseline "Software-Based ECDSA P-256" TERBERAT
// (kelompok K1/K3, Bab 3.3.1 butir a.1). Dipasang lewat build_flag:
//     -DMBEDTLS_CONFIG_FILE='"mbedtls_heavy_config.h"'
//
// Prinsip: hanya modul yang dibutuhkan ECDSA P-256 yang dikompilasi, dan SEMUA
// optimasi kecepatan opsional dimatikan (profil "portable C"). Setiap saklar di
// bawah adalah opsi resmi Mbed TLS (lihat mbedtls_config.h upstream), bukan
// perubahan pada kode kriptografinya.
//
//   Saklar                         Default upstream      Di sini
//   MBEDTLS_HAVE_ASM               aktif                 MATI   -> bignum murni C
//   MBEDTLS_ECP_NIST_OPTIM         aktif                 MATI   -> reduksi mod p umum
//                                  ("4 s.d. 8 kali lebih cepat" menurut dokumentasi
//                                   upstream) diganti pembagian bignum generik
//   MBEDTLS_ECP_FIXED_POINT_OPTIM  1 (tabel comb G)      0      -> tanpa tabel
//                                  pra-hitung titik basis (juga membuat sampel ke-1
//                                  dan ke-30 berlatensi sama, tidak ada efek cache)
//   MBEDTLS_ECP_WINDOW_SIZE        4 (G: 6)              2      -> ukuran window comb
//                                  minimum (lebih banyak penjumlahan titik)
//
// Untuk membandingkan dengan Mbed TLS "default" (rentang bawah), definisikan
//     -DMBEDTLS_PROFILE_DEFAULT
// pada build_flags: ketiga optimasi di atas dikembalikan ke nilai upstream.
//
// Keamanan runtime tidak dilonggarkan: randomisasi koordinat Z titik, blinding,
// dan pemeriksaan kunci tetap aktif (default upstream).
// ============================================================================
#ifndef MBEDTLS_HEAVY_CONFIG_H
#define MBEDTLS_HEAVY_CONFIG_H

/* --- Modul minimum untuk ECDSA secp256r1 (sign + verify) --- */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY          /* hook calloc/free -> ukur puncak heap per operasi */
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ASN1_PARSE_C             /* prasyarat check_config.h untuk ECDSA_C */
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED /* hanya NIST P-256 */

/* --- Lingkungan bare-metal --- */
#define MBEDTLS_NO_PLATFORM_ENTROPY      /* nonce k dari RNG milik firmware (lihat .cpp) */

/* --- Profil kecepatan --- */
#if defined(MBEDTLS_PROFILE_DEFAULT)
  #define MBEDTLS_HAVE_ASM
  #define MBEDTLS_ECP_NIST_OPTIM
  /* MBEDTLS_ECP_FIXED_POINT_OPTIM dan MBEDTLS_ECP_WINDOW_SIZE: nilai upstream */
#else
  /* MBEDTLS_HAVE_ASM dan MBEDTLS_ECP_NIST_OPTIM sengaja TIDAK didefinisikan */
  #define MBEDTLS_ECP_FIXED_POINT_OPTIM 0
  #define MBEDTLS_ECP_WINDOW_SIZE       2
#endif

#endif /* MBEDTLS_HEAVY_CONFIG_H */
