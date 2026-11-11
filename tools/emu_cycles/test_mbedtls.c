#include "common.h"
#include <stdlib.h>
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/platform.h"
uint8_t g_sig[64]; int32_t g_ret = -999; uint32_t g_stack, g_heap_sbrk, g_heap_peak;

static uint32_t cur, peak;
static void *trk_calloc(size_t n, size_t sz) {
  size_t b = n * sz; uint32_t *p = (uint32_t *)calloc(1, b + 8);
  if (!p) return NULL; p[0] = (uint32_t)b; cur += b; if (cur > peak) peak = cur; return p + 2; }
static void trk_free(void *q) { if (!q) return; uint32_t *p = (uint32_t *)q - 2; cur -= p[0]; free(p); }
static int rng_cb(void *u, unsigned char *d, size_t n) { (void)u; rng_fill(d, n); return 0; }

int main(void) {
  mbedtls_ecp_group grp; mbedtls_mpi d, r, s;
  mbedtls_platform_set_calloc_free(trk_calloc, trk_free);
  mbedtls_ecp_group_init(&grp); mbedtls_mpi_init(&d); mbedtls_mpi_init(&r); mbedtls_mpi_init(&s);
  if (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1)) { g_ret = -1; return 0; }
  mbedtls_mpi_read_binary(&d, PRIVATE_KEY, 32);
  uint32_t base = cur; peak = cur;
  stack_paint();
  marker_begin();
  int ret = mbedtls_ecdsa_sign(&grp, &r, &s, &d, DIGEST, 32, rng_cb, NULL);
  marker_end();
  g_stack = stack_used(); g_heap_sbrk = heap_used_sbrk(); g_heap_peak = peak - base;
  if (!ret) { mbedtls_mpi_write_binary(&r, g_sig, 32); mbedtls_mpi_write_binary(&s, g_sig + 32, 32); }
  g_ret = ret; return 0;
}
