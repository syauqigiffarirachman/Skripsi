/* DS28E38 command layer (port of ds9481/ds28e38.py and Maxim ds28e38.c).
 * Every command is an "XPC" frame:
 *   reset + ROM command
 *   TX 0x66 | length | sub-command | params ...  -> RX inverted CRC16
 *   TX release byte 0xAA with 5 V strong pull-up while the chip works
 *   RX dummy | length | result | data ... | inverted CRC16
 * Result byte 0xAA = success. */
#ifndef DS28E38_H
#define DS28E38_H

#include <stddef.h>
#include <stdint.h>

#include "onewire.h"

/* return codes (0 = success) */
#define DS_OK           0
#define DS_E_COMM      (-1)  /* no/short answer from the adapter */
#define DS_E_CRC       (-2)  /* CRC16 of an XPC frame is wrong */
#define DS_E_DEVICE    (-3)  /* chip returned a result byte other than 0xAA (see last_result) */
#define DS_E_LEN       (-4)  /* unexpected response length */
#define DS_E_PRESENCE  (-5)  /* no presence pulse: nothing on the bus */
#define DS_E_PARAM     (-6)  /* bad argument */
#define DS_E_PERMANENT (-7)  /* irreversible operation not confirmed */

#define DS_SELECT_SKIP   0   /* one device on the bus */
#define DS_SELECT_MATCH  1   /* use dev->rom */
#define DS_SELECT_RESUME 2

#define DS_PG_PUB_KEY_X 4
#define DS_PG_PUB_KEY_Y 5
#define DS_PG_PRIV_KEY  6

typedef struct {
    ow_t    *ow;
    int      select;
    uint8_t  rom[8];
    uint8_t  manid[2];
    int      have_rom, have_manid;
    uint8_t  last_result;  /* result byte of the last command */
} ds28e38_t;

typedef struct {
    uint8_t protection[7];  /* pages 0-6 */
    uint8_t manid[2];
    uint8_t rom_version[2];
    uint8_t extra;
} ds28e38_status_t;

const char *ds_strerror(int code);
void ds28e38_init(ds28e38_t *d, ow_t *ow);

int ds28e38_read_status(ds28e38_t *d, ds28e38_status_t *st);
int ds28e38_read_memory(ds28e38_t *d, int page, uint8_t out[32]);      /* pages 0-6 */
int ds28e38_write_memory(ds28e38_t *d, int page, const uint8_t data[32]);
int ds28e38_read_rng(ds28e38_t *d, int n, uint8_t *out);               /* n = 1..64 */
int ds28e38_decrement_counter(ds28e38_t *d);
/* ECDSA signature of SHA-256(ROM | page data | challenge | page# | MANID).
 * The chip sends s first, then r; both returned MSByte first. */
int ds28e38_compute_read_page_auth(ds28e38_t *d, int page, const uint8_t challenge[32],
                                   int anonymous, uint8_t r[32], uint8_t s[32]);
/* status + ROM number + MANID */
int ds28e38_detect(ds28e38_t *d, ds28e38_status_t *st);

/* PERMANENT operations: refused unless confirm_permanent != 0 */
int ds28e38_set_page_protection(ds28e38_t *d, int page, uint8_t prot, int confirm_permanent);
int ds28e38_generate_key_pair(ds28e38_t *d, int use_puf, int lock, int confirm_permanent);

#endif
