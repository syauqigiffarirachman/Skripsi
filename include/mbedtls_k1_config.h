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
//   MBEDTLS_ECP_FIXED_POINT_OPTIM  DIMATIKAN sejak 13 Nov (tahap 1c; default upstream = 1)
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

/* --- Hardening (13 Nov): hasil fungsi Mbed TLS tidak boleh diabaikan diam-diam --- */
#define MBEDTLS_CHECK_RETURN_WARNING

/* --- Tahap 1c (13 Nov): MBEDTLS_ECP_FIXED_POINT_OPTIM = 0 (tabel comb titik basis G dimatikan) ---
 * Opsi ini mematikan CACHE tabel presomputasi G (grp->T). Algoritma perkalian tetap comb,
 * tetapi tabelnya dihitung ulang di setiap sign (d x doublings) dan window turun dari 5 ke 4.
 * Terukur di board (build bersih): sign 278 ms -> 654,6 ms (2,35x lebih lambat).
 * Pemilihan entri tabel di Mbed TLS sudah konstan-waktu (ecp_select_comb) di kedua mode, jadi
 * ini BUKAN mitigasi side-channel tersendiri; jangan diklaim begitu di skripsi.
 * PERINGATAN BUILD: konfigurasi dimuat lewat -DMBEDTLS_CONFIG_FILE=..., sehingga SCons TIDAK
 * membangun ulang lib/mbedtls bila file ini berubah. Setelah mengubah opsi di sini selalu
 *   pio run -e k1_sw -t clean   sebelum build, atau hasil ukur memakai library lama.
 * MBEDTLS_ECP_WINDOW_SIZE tidak diubah (default 4). */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM 0

/* --- Optimasi kecepatan upstream: aktif (kecuali comb di atas) --- */
/* Saklar ukur (hanya untuk eksperimen pengaruh per opsi, tidak dipakai di baseline):
 *   PLATFORMIO_BUILD_FLAGS=-DK1_NO_ASM   atau   -DK1_NO_NIST */
#if !defined(K1_NO_ASM)
#define MBEDTLS_HAVE_ASM
#endif
#if !defined(K1_NO_NIST)
#define MBEDTLS_ECP_NIST_OPTIM
#endif
/* MBEDTLS_ECP_WINDOW_SIZE: nilai default upstream */

#endif /* MBEDTLS_K1_CONFIG_H */
