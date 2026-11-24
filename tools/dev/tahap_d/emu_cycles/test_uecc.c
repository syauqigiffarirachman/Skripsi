#include "common.h"
#include "uECC.h"
uint8_t g_sig[64]; int32_t g_ret = -999; uint32_t g_stack, g_heap_sbrk, g_heap_peak;
static int rng_cb(uint8_t *d, unsigned n) { rng_fill(d, n); return 1; }
int main(void) {
  uECC_set_rng(rng_cb);
  uECC_Curve c = uECC_secp256r1();
  stack_paint();
  marker_begin();
  int ok = uECC_sign(PRIVATE_KEY, DIGEST, 32, g_sig, c);
  marker_end();
  g_stack = stack_used(); g_heap_sbrk = heap_used_sbrk(); g_heap_peak = 0;
  g_ret = ok ? 0 : -1; return 0;
}
