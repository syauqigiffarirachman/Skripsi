/* NIST P-256 ECDSA signature verification (no external library).
 * Verification only touches public data, so it is not constant-time. */
#ifndef P256_H
#define P256_H

#include <stdint.h>

/* 1 = valid, 0 = invalid. All values are 32-byte big-endian. digest = SHA-256 of the message. */
int p256_ecdsa_verify(const uint8_t pub_x[32], const uint8_t pub_y[32],
                      const uint8_t digest[32], const uint8_t r[32], const uint8_t s[32]);

#endif
