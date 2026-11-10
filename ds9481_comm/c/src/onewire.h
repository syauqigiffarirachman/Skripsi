/* 1-Wire master over the DS9481 (DS2480B-style serial protocol).
 * Port of ds9481/onewire.py. The adapter is either in COMMAND mode (0xE3:
 * bytes are adapter commands) or DATA mode (0xE1: bytes go onto the bus). */
#ifndef ONEWIRE_H
#define ONEWIRE_H

#include <stddef.h>
#include <stdint.h>

#include "serial_win32.h"

#define OW_MODE_DATA     0xE1
#define OW_MODE_COMMAND  0xE3
#define OW_LEVEL_NORMAL  0
#define OW_LEVEL_STRONG5 2
#define OW_MAX_BLOCK     160

typedef struct {
    uint8_t raw[64];  /* [length][data...][crc8], same layout as the Maxim ver[] array */
    size_t  len;
    int     crc_ok;
    int     major, minor;
    int     i2c_supported;
} ow_version_t;

typedef struct {
    serial_t ser;
    uint8_t  mode;   /* OW_MODE_COMMAND / OW_MODE_DATA */
    int      level;  /* OW_LEVEL_NORMAL / OW_LEVEL_STRONG5 */
    uint8_t  speed;  /* SPEEDSEL_FLEX */
    uint8_t  baud;   /* baud-rate code */
    ow_version_t ver; /* adapter firmware info, filled by ow_open() */
} ow_t;

/* Find and open the adapter. port = "COM13" or NULL to scan COM1..COM256.
 * On success returns 0, the adapter is in 1-Wire mode and found_port holds its name.
 * Recovers an adapter left in I2C mode. */
int  ow_open(ow_t *ow, const char *port, int debug, char *found_port, size_t found_size);
void ow_close(ow_t *ow);

int  ow_detect(ow_t *ow);                       /* 1 = adapter answered */
int  ow_read_version(ow_t *ow, ow_version_t *v);/* 0 ok, -1 fail */
int  ow_level_normal(ow_t *ow);                 /* 0 ok, -1 fail */
int  ow_write_byte_power(ow_t *ow, uint8_t v);  /* 1 = echo ok, 0 = mismatch, -1 = comm error */

int  ow_reset(ow_t *ow);                        /* 1 = presence pulse, 0 = none/error */
int  ow_touch_byte(ow_t *ow, uint8_t v, uint8_t *out);  /* 0 ok, -1 fail */
int  ow_block(ow_t *ow, uint8_t *data, size_t n);       /* in place; 0 ok, -1 fail, -2 too long */

int  ow_skip_rom(ow_t *ow);                     /* 1 = ok (presence), 0 = no device */
int  ow_resume(ow_t *ow);
int  ow_match_rom(ow_t *ow, const uint8_t rom[8]);
int  ow_read_rom(ow_t *ow, uint8_t rom[8]);     /* 0 ok, -1 absent / CRC8 wrong */

#endif
