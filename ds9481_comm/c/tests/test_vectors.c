/* Tes offline (tanpa hardware): SHA-256, CRC, dan verifikasi ECDSA P-256.
 * Vektor ECDSA = tanda tangan NYATA yang dibuat DS28E38 (lihat tanda_tangan.json). */
#include <stdio.h>
#include <string.h>

#include "../src/crc.h"
#include "../src/p256.h"
#include "../src/sha256.h"

static int failures;

static void hex(const char *s, uint8_t *out, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        unsigned v;
        sscanf(s + 2 * i, "%2x", &v);
        out[i] = (uint8_t)v;
    }
}

static void check(const char *name, int cond)
{
    printf("%-52s %s\n", name, cond ? "OK" : "GAGAL");
    if (!cond)
        failures++;
}

int main(void)
{
    uint8_t d[32], want[32];
    uint8_t px[32], py[32], digest[32], r[32], s[32], t[32];
    uint8_t body[7] = {0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00};
    uint8_t frame[44];
    uint16_t inv;
    int i;

    /* SHA-256 (FIPS 180-4 examples) */
    sha256("abc", 3, d);
    hex("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", want, 32);
    check("SHA-256('abc')", memcmp(d, want, 32) == 0);
    sha256("", 0, d);
    hex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", want, 32);
    check("SHA-256('')", memcmp(d, want, 32) == 0);
    {
        const char *m = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        sha256(m, strlen(m), d);
        hex("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", want, 32);
        check("SHA-256(56 byte, dua blok)", memcmp(d, want, 32) == 0);
    }

    /* CRC */
    check("CRC8 02 1C B8 01 00 00 00 = A2", crc8(body, 7, 0) == 0xA2);
    for (i = 0; i < 40; i++)
        frame[i] = (uint8_t)i;
    inv = (uint16_t)~crc16(frame, 40, 0);
    frame[40] = (uint8_t)(inv & 0xFF);
    frame[41] = (uint8_t)(inv >> 8);
    check("CRC16 sisa = 0xB001 (data + CRC terbalik)", crc16(frame, 42, 0) == CRC16_RESIDUE);

    /* ECDSA P-256: signature produced by the real chip */
    hex("87a51156633da60bf0f4e95765192edc3280dea89ffd6e62b247d9f95b0bb85c", px, 32);
    hex("35d06ce2f647220466056152d5f814c087cc99e2fda9f43884f86cc169b79e4f", py, 32);
    hex("f7f5c67554d88326f659bd80b5f6e9525f40e7ca78fe2fb7f1b842379d07769a", digest, 32);
    hex("726dcd54e342434aa2eca8d66e94d8e4eb49ae3ae83e0946041df0e8c4f66850", r, 32);
    hex("dd4864c239279ff4fa1bf0d0dc3b53013d9c34e608b65a468edfc07e063ff913", s, 32);
    check("ECDSA: tanda tangan chip VALID", p256_ecdsa_verify(px, py, digest, r, s) == 1);

    memcpy(t, s, 32); t[31] ^= 0x01;
    check("ECDSA: s diubah 1 bit -> ditolak", p256_ecdsa_verify(px, py, digest, r, t) == 0);
    memcpy(t, r, 32); t[0] ^= 0x80;
    check("ECDSA: r diubah -> ditolak", p256_ecdsa_verify(px, py, digest, t, s) == 0);
    memcpy(t, digest, 32); t[10] ^= 0x04;
    check("ECDSA: hash diubah -> ditolak", p256_ecdsa_verify(px, py, t, r, s) == 0);
    memcpy(t, py, 32); t[31] ^= 0x01;
    check("ECDSA: kunci publik tidak di kurva -> ditolak", p256_ecdsa_verify(px, t, digest, r, s) == 0);
    memset(t, 0, 32);
    check("ECDSA: r = 0 -> ditolak", p256_ecdsa_verify(px, py, digest, t, s) == 0);
    check("ECDSA: s = 0 -> ditolak", p256_ecdsa_verify(px, py, digest, r, t) == 0);

    printf("\n%s\n", failures ? "ADA YANG GAGAL" : "SEMUA OK");
    return failures ? 1 : 0;
}
