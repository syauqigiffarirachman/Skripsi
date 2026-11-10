#include "ds28e38.h"

#include <string.h>

#include "crc.h"

#define XPC_COMMAND        0x66
#define CMD_WRITE_MEM      0x96
#define CMD_READ_MEM       0x44  /* value from Maxim's ds28e38.h */
#define CMD_READ_STATUS    0xAA
#define CMD_SET_PAGE_PROT  0xC3
#define CMD_COMP_READ_AUTH 0xA5
#define CMD_DECREMENT_CNT  0xC9
#define CMD_GEN_ECDSA_KEY  0xCB
#define CMD_READ_RNG       0xD2
#define RESULT_SUCCESS     0xAA
#define ECDSA_KEY_LOCK     0x80
#define ECDSA_USE_PUF      0x01

/* processing delays (ms), 128x PUF oversampling values from ds28e38.h */
#define DELAY_WRITE 180
#define DELAY_READ  120
#define DELAY_ECDSA 520
#define DELAY_TCMP  25

const char *ds_strerror(int code)
{
    switch (code) {
    case DS_OK:           return "OK";
    case DS_E_COMM:       return "tidak ada jawaban dari adaptor/chip (timeout)";
    case DS_E_CRC:        return "CRC16 salah";
    case DS_E_DEVICE:     return "chip mengembalikan kode kesalahan";
    case DS_E_LEN:        return "panjang respons tidak sesuai";
    case DS_E_PRESENCE:   return "tidak ada presence pulse (chip tidak terhubung?)";
    case DS_E_PARAM:      return "parameter tidak valid";
    case DS_E_PERMANENT:  return "operasi permanen belum dikonfirmasi";
    default:              return "kesalahan tidak dikenal";
    }
}

void ds28e38_init(ds28e38_t *d, ow_t *ow)
{
    memset(d, 0, sizeof *d);
    d->ow = ow;
    d->select = DS_SELECT_SKIP;
}

static int select_device(ds28e38_t *d)
{
    int ok;
    switch (d->select) {
    case DS_SELECT_MATCH:
        if (!d->have_rom)
            return DS_E_PARAM;
        ok = ow_match_rom(d->ow, d->rom);
        break;
    case DS_SELECT_RESUME:
        ok = ow_resume(d->ow);
        break;
    default:
        ok = ow_skip_rom(d->ow);
        break;
    }
    return ok ? DS_OK : DS_E_PRESENCE;
}

/* One XPC command. sub = sub-command + parameters. The bytes after the result
 * byte (expect_len - 1 of them) are copied to out (may be NULL). */
static int xpc(ds28e38_t *d, const uint8_t *sub, size_t sublen, int delay_ms,
               size_t expect_len, uint8_t *out)
{
    uint8_t frame[2 + 64 + 2];
    uint8_t hdr[2];
    uint8_t body[OW_MAX_BLOCK];
    size_t n, flen;
    int rc;

    if (sublen == 0 || sublen > 64)
        return DS_E_PARAM;
    rc = select_device(d);
    if (rc != DS_OK)
        return rc;
    d->last_result = 0;

    frame[0] = XPC_COMMAND;
    frame[1] = (uint8_t)sublen;
    memcpy(&frame[2], sub, sublen);
    frame[2 + sublen] = 0xFF;      /* the chip answers with the inverted CRC16 here */
    frame[3 + sublen] = 0xFF;
    flen = 4 + sublen;
    if (ow_block(d->ow, frame, flen) != 0)
        return DS_E_COMM;
    if (crc16(frame, flen, 0) != CRC16_RESIDUE)
        return DS_E_CRC;

    ow_write_byte_power(d->ow, 0xAA);  /* release byte + strong pull-up (result ignored, as Maxim) */
    if (delay_ms)
        ser_sleep(delay_ms);
    if (ow_level_normal(d->ow) != 0)
        return DS_E_COMM;

    hdr[0] = hdr[1] = 0xFF;            /* dummy byte, length byte */
    if (ow_block(d->ow, hdr, 2) != 0)
        return DS_E_COMM;
    n = hdr[1];
    if (n == 0 || n == 0xFF || n + 2 > OW_MAX_BLOCK)
        return DS_E_LEN;
    memset(body, 0xFF, n + 2);
    if (ow_block(d->ow, body, n + 2) != 0)
        return DS_E_COMM;
    {
        uint16_t c = crc16(&hdr[1], 1, 0);
        c = crc16(body, n + 2, c);
        if (c != CRC16_RESIDUE)
            return DS_E_CRC;
    }
    d->last_result = body[0];
    if (body[0] != RESULT_SUCCESS)
        return DS_E_DEVICE;
    if (n != expect_len)
        return DS_E_LEN;
    if (out && n > 1)
        memcpy(out, &body[1], n - 1);
    return DS_OK;
}

int ds28e38_read_status(ds28e38_t *d, ds28e38_status_t *st)
{
    const uint8_t sub[2] = {CMD_READ_STATUS, 0};
    uint8_t b[12];
    int rc = xpc(d, sub, 2, DELAY_READ, 13, b);
    if (rc != DS_OK)
        return rc;
    memcpy(st->protection, &b[0], 7);
    memcpy(st->manid, &b[7], 2);
    memcpy(st->rom_version, &b[9], 2);
    st->extra = b[11];
    return DS_OK;
}

int ds28e38_read_memory(ds28e38_t *d, int page, uint8_t out[32])
{
    uint8_t sub[2];
    sub[0] = CMD_READ_MEM;
    sub[1] = (uint8_t)(page & 0x07);
    return xpc(d, sub, 2, DELAY_READ, 33, out);
}

int ds28e38_write_memory(ds28e38_t *d, int page, const uint8_t data[32])
{
    uint8_t sub[34];
    sub[0] = CMD_WRITE_MEM;
    sub[1] = (uint8_t)(page & 0x07);
    memcpy(&sub[2], data, 32);
    return xpc(d, sub, sizeof sub, DELAY_WRITE, 1, NULL);
}

int ds28e38_read_rng(ds28e38_t *d, int n, uint8_t *out)
{
    uint8_t sub[2];
    if (n < 1 || n > 64)
        return DS_E_PARAM;
    sub[0] = CMD_READ_RNG;
    sub[1] = (uint8_t)(n - 1);  /* chip is asked for n-1 and returns n bytes */
    return xpc(d, sub, 2, DELAY_TCMP, (size_t)n + 1, out);
}

int ds28e38_decrement_counter(ds28e38_t *d)
{
    const uint8_t sub[1] = {CMD_DECREMENT_CNT};
    return xpc(d, sub, 1, DELAY_WRITE, 1, NULL);
}

int ds28e38_compute_read_page_auth(ds28e38_t *d, int page, const uint8_t challenge[32],
                                   int anonymous, uint8_t r[32], uint8_t s[32])
{
    uint8_t sub[34], sig[64];
    int rc;
    sub[0] = CMD_COMP_READ_AUTH;
    sub[1] = (uint8_t)((page & 0x07) | (anonymous ? 0xE0 : 0));
    memcpy(&sub[2], challenge, 32);
    rc = xpc(d, sub, sizeof sub, DELAY_ECDSA, 65, sig);
    if (rc != DS_OK)
        return rc;
    memcpy(s, &sig[0], 32);   /* chip sends s first ... */
    memcpy(r, &sig[32], 32);  /* ... then r */
    return DS_OK;
}

int ds28e38_detect(ds28e38_t *d, ds28e38_status_t *st)
{
    ds28e38_status_t tmp;
    int rc;
    if (!st)
        st = &tmp;
    rc = ds28e38_read_status(d, st);
    if (rc != DS_OK)
        return rc;
    memcpy(d->manid, st->manid, 2);
    d->have_manid = 1;
    if (ow_read_rom(d->ow, d->rom) == 0)
        d->have_rom = 1;
    return DS_OK;
}

int ds28e38_set_page_protection(ds28e38_t *d, int page, uint8_t prot, int confirm_permanent)
{
    uint8_t sub[3];
    if (!confirm_permanent)
        return DS_E_PERMANENT;
    sub[0] = CMD_SET_PAGE_PROT;
    sub[1] = (uint8_t)(page & 0x07);
    sub[2] = prot;
    return xpc(d, sub, 3, DELAY_WRITE, 1, NULL);
}

int ds28e38_generate_key_pair(ds28e38_t *d, int use_puf, int lock, int confirm_permanent)
{
    uint8_t sub[2];
    if (!confirm_permanent)
        return DS_E_PERMANENT;
    sub[0] = CMD_GEN_ECDSA_KEY;
    sub[1] = (uint8_t)((lock ? ECDSA_KEY_LOCK : 0) | (use_puf ? ECDSA_USE_PUF : 0));
    return xpc(d, sub, 2, DELAY_ECDSA, 1, NULL);
}
