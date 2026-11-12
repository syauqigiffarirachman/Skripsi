// ============================================================================
// Konfigurasi Mbed TLS 3.6.7 untuk baseline K1/K3 "Software-Based ECDSA P-256" (paper Bab 3).
// Dipasang lewat build_flag:  -DMBEDTLS_CONFIG_FILE='"mbedtls_k1_config.h"'
//
// Keputusan 2026-11-12: baseline = ECDSA STANDAR (nonce deterministik RFC 6979) dengan
// optimasi upstream AKTIF. Bukan profil heavy (itu tetap ada di mbedtls_heavy_config.h
// sebagai batas atas, tidak dipakai di sini).
//
// Optimasi yang aktif (nilai default upstream, tidak ada yang dimatikan):
//   MBEDTLS_HAVE_ASM               bignum memakai assembly ARM (bn_mul.h)
//   MBEDTLS_ECP_NIST_OPTIM         reduksi modulo cepat khusus kurva NIST
//   MBEDTLS_ECP_FIXED_POINT_OPTIM  tabel comb untuk titik basis G (nilai default 1)
//   MBEDTLS_ECP_WINDOW_SIZE        nilai default 4
// Keamanan runtime tidak dilonggarkan (blinding, randomisasi koordinat Z tetap aktif).
// ============================================================================
#ifndef MBEDTLS_K1_CONFIG_H
#define MBEDTLS_K1_CONFIG_H

/* --- Modul minimum untuk ECDSA secp256r1 deterministik (RFC 6979) --- */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ASN1_PARSE_C             /* prasyarat check_config.h untuk ECDSA_C */
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED /* hanya NIST P-256 */

/* --- Nonce deterministik RFC 6979: HMAC-DRBG di atas HMAC-SHA-256 --- */
#define MBEDTLS_ECDSA_DETERMINISTIC
#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_MD_C
#define MBEDTLS_SHA256_C

/* --- Lingkungan bare-metal --- */
#define MBEDTLS_NO_PLATFORM_ENTROPY      /* RNG hanya untuk blinding, milik firmware */

/* --- Optimasi kecepatan upstream: aktif --- */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_ECP_NIST_OPTIM
/* MBEDTLS_ECP_FIXED_POINT_OPTIM dan MBEDTLS_ECP_WINDOW_SIZE: nilai default upstream */

#endif /* MBEDTLS_K1_CONFIG_H */
