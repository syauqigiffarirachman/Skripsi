#include "onewire.h"

#include <stdio.h>
#include <string.h>

#include "crc.h"

/* command byte building blocks (see Maxim 1wire_ds9481.h) */
#define CMD_COMM            0x81
#define CMD_CONFIG          0x01
#define FUNCTSEL_BIT        0x00
#define FUNCTSEL_RESET      0x40
#define FUNCTSEL_CHMOD      0x60
#define BITPOL_ONE          0x10
#define BITPOL_ZERO         0x00
#define SPEEDSEL_FLEX       0x04
#define SPEEDSEL_PULSE      0x0C
#define PRIME5V_TRUE        0x02
#define MODE_STOP_PULSE     0xF1

#define PARMSEL_PARMREAD    0x00
#define PARMSEL_VERSION_EX  0x00
#define PARMSEL_SLEW        0x10
#define PARMSEL_5VPULSE     0x30
#define PARMSEL_WRITE1LOW   0x40
#define PARMSEL_SAMPLEOFF   0x50
#define PARMSEL_BAUDRATE    0x70
#define PARMSET_INFINITE    0x0E
#define PARMSET_SLEW1P37    0x06
#define PARMSET_WRITE10US   0x04
#define PARMSET_SAMPOFF8US  0x0A

#define RB_RESET_MASK       0x03
#define RB_PRESENCE         0x01
#define RB_ALARMPRESENCE    0x02

/* little packet builder */
typedef struct {
    uint8_t b[400];
    size_t  n;
} pkt_t;

static void need(ow_t *ow, pkt_t *p, uint8_t mode)
{
    if (ow->mode != mode) {
        ow->mode = mode;
        p->b[p->n++] = mode;
    }
}

/* flush, send, read exactly nread bytes. On failure re-sync the adapter. */
static int txrx(ow_t *ow, const pkt_t *p, uint8_t *rx, size_t nread)
{
    ser_flush(&ow->ser);
    if (ser_write(&ow->ser, p->b, p->n)) {
        if (nread == 0)
            return 0;
        if (ser_read(&ow->ser, rx, nread) == nread)
            return 0;
    }
    ow_detect(ow);
    return -1;
}

int ow_detect(ow_t *ow)
{
    static const uint8_t timing = 0xC1;
    uint8_t cfg[5], rx[5];

    ow->mode = OW_MODE_COMMAND;
    ow->baud = DS_BAUD_9600;
    ow->speed = SPEEDSEL_FLEX;
    ow->level = OW_LEVEL_NORMAL;
    ser_set_baud(&ow->ser, ow->baud);
    ser_break(&ow->ser, 2);
    ser_sleep(2);
    ser_flush(&ow->ser);
    if (!ser_write(&ow->ser, &timing, 1))
        return 0;
    ser_sleep(2);

    cfg[0] = CMD_CONFIG | PARMSEL_SLEW | PARMSET_SLEW1P37;                   /* PDSRC 1.37 V/us */
    cfg[1] = CMD_CONFIG | PARMSEL_WRITE1LOW | PARMSET_WRITE10US;             /* W1LT 10 us */
    cfg[2] = CMD_CONFIG | PARMSEL_SAMPLEOFF | PARMSET_SAMPOFF8US;            /* DSO 8 us */
    cfg[3] = CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3);        /* read baud */
    cfg[4] = CMD_COMM | FUNCTSEL_BIT | ow->baud | BITPOL_ONE;                /* 1-bit self test */
    ser_flush(&ow->ser);
    if (ser_write(&ow->ser, cfg, sizeof cfg) && ser_read(&ow->ser, rx, 5) == 5) {
        return (rx[3] & 0xF1) == 0x00 && (rx[3] & 0x0E) == ow->baud &&
               (rx[4] & 0xF0) == 0x90 && (rx[4] & 0x0C) == ow->baud;
    }
    return 0;
}

int ow_read_version(ow_t *ow, ow_version_t *v)
{
    pkt_t p = {{0}, 0};
    uint8_t ln;
    int ver;

    memset(v, 0, sizeof *v);
    need(ow, &p, OW_MODE_COMMAND);
    p.b[p.n++] = CMD_CONFIG | PARMSEL_PARMREAD | PARMSEL_VERSION_EX;
    ser_flush(&ow->ser);
    if (!ser_write(&ow->ser, p.b, p.n))
        return -1;
    if (ser_read(&ow->ser, &ln, 1) != 1 || ln == 0 || (size_t)ln + 1 >= 60)
        return -1;
    v->raw[0] = ln;
    if (ser_read(&ow->ser, &v->raw[1], (size_t)ln + 1) != (size_t)ln + 1)
        return -1;
    v->len = (size_t)ln + 2;
    v->crc_ok = crc8(v->raw, v->len, 0) == 0;
    ver = v->len > 3 ? v->raw[3] : 0;
    v->major = ver >> 4;
    v->minor = ver & 0x0F;
    v->i2c_supported = v->len > 11 && (v->raw[11] & 0x10) == 0x10;
    return 0;
}

int ow_level_normal(ow_t *ow)
{
    pkt_t p = {{0}, 0};
    uint8_t rx[2];

    if (ow->level == OW_LEVEL_NORMAL)
        return 0;
    need(ow, &p, OW_MODE_COMMAND);
    p.b[p.n++] = MODE_STOP_PULSE;
    p.b[p.n++] = CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE;  /* pulse without prime */
    p.b[p.n++] = MODE_STOP_PULSE;
    if (txrx(ow, &p, rx, 2) != 0)
        return -1;
    if ((rx[0] & 0xE0) == 0xE0 && (rx[1] & 0xE0) == 0xE0) {
        ow->level = OW_LEVEL_NORMAL;
        return 0;
    }
    ow_detect(ow);
    return -1;
}

int ow_write_byte_power(ow_t *ow, uint8_t v)
{
    pkt_t p = {{0}, 0};
    uint8_t rx[9], tmp = v, echo = 0;
    int i;

    need(ow, &p, OW_MODE_COMMAND);
    p.b[p.n++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_INFINITE;  /* SPUD time */
    for (i = 0; i < 8; i++) {
        uint8_t b = (tmp & 1) ? BITPOL_ONE : BITPOL_ZERO;
        b |= CMD_COMM | FUNCTSEL_BIT | ow->speed;
        if (i == 7)
            b |= PRIME5V_TRUE;  /* strong pull-up after the last bit */
        p.b[p.n++] = b;
        tmp >>= 1;
    }
    if (txrx(ow, &p, rx, 9) != 0)
        return -1;
    if ((rx[0] & 0x81) != 0)
        return 0;
    ow->level = OW_LEVEL_STRONG5;
    for (i = 0; i < 8; i++) {
        echo >>= 1;
        if (rx[i + 1] & 0x01)
            echo |= 0x80;
    }
    return echo == v;
}

int ow_reset(ow_t *ow)
{
    pkt_t p = {{0}, 0};
    uint8_t rx;

    if (ow_level_normal(ow) != 0)
        return 0;
    need(ow, &p, OW_MODE_COMMAND);
    p.b[p.n++] = CMD_COMM | FUNCTSEL_RESET | ow->speed;
    if (txrx(ow, &p, &rx, 1) != 0)
        return 0;
    return (rx & RB_RESET_MASK) == RB_PRESENCE || (rx & RB_RESET_MASK) == RB_ALARMPRESENCE;
}

int ow_touch_byte(ow_t *ow, uint8_t v, uint8_t *out)
{
    pkt_t p = {{0}, 0};
    uint8_t rx;

    if (ow_level_normal(ow) != 0)
        return -1;
    need(ow, &p, OW_MODE_DATA);
    p.b[p.n++] = v;
    if (v == OW_MODE_COMMAND)
        p.b[p.n++] = v;  /* escape: a data byte equal to 0xE3 is doubled */
    if (txrx(ow, &p, &rx, 1) != 0)
        return -1;
    if (out)
        *out = rx;
    return 0;
}

int ow_block(ow_t *ow, uint8_t *data, size_t n)
{
    pkt_t p = {{0}, 0};
    size_t i;

    if (n > OW_MAX_BLOCK)
        return -2;
    if (ow_level_normal(ow) != 0)
        return -1;
    need(ow, &p, OW_MODE_DATA);
    for (i = 0; i < n; i++) {
        p.b[p.n++] = data[i];
        if (data[i] == OW_MODE_COMMAND)
            p.b[p.n++] = data[i];
    }
    return txrx(ow, &p, data, n);
}

int ow_skip_rom(ow_t *ow)
{
    if (!ow_reset(ow))
        return 0;
    return ow_touch_byte(ow, 0xCC, NULL) == 0;
}

int ow_resume(ow_t *ow)
{
    if (!ow_reset(ow))
        return 0;
    return ow_touch_byte(ow, 0xA5, NULL) == 0;
}

int ow_match_rom(ow_t *ow, const uint8_t rom[8])
{
    uint8_t buf[9];

    if (!ow_reset(ow))
        return 0;
    buf[0] = 0x55;
    memcpy(&buf[1], rom, 8);
    return ow_block(ow, buf, 9) == 0;
}

int ow_read_rom(ow_t *ow, uint8_t rom[8])
{
    uint8_t buf[9];

    if (!ow_reset(ow))
        return -1;
    buf[0] = 0x33;
    memset(&buf[1], 0xFF, 8);
    if (ow_block(ow, buf, 9) != 0)
        return -1;
    if (crc8(&buf[1], 8, 0) == 0 && buf[1] != 0) {
        memcpy(rom, &buf[1], 8);
        return 0;
    }
    return -1;
}

/* Detect the adapter in 1-Wire mode; if it does not answer assume it is stuck in
 * I2C mode (115200 baud) from an earlier run and bring it back with 'C','O',
 * then try a DTR power-on reset. */
static int detect_with_recovery(ow_t *ow)
{
    if (ow_detect(ow))
        return 1;
    ser_set_baud(&ow->ser, DS_BAUD_115200);
    ser_write(&ow->ser, (const uint8_t *)"C", 1);
    ser_write(&ow->ser, (const uint8_t *)"O", 1);
    ser_flush(&ow->ser);
    ser_sleep(500);
    ser_set_baud(&ow->ser, DS_BAUD_9600);
    if (ow_detect(ow))
        return 1;
    ser_set_dtr(&ow->ser, 0);  /* power-on reset (Maxim DS9481_POR) */
    ser_sleep(500);
    ser_set_dtr(&ow->ser, 1);
    ser_sleep(500);
    return ow_detect(ow);
}

static int try_port(ow_t *ow, const char *port, int debug)
{
    ow_version_t v;

    memset(ow, 0, sizeof *ow);
    ow->ser.debug = debug;
    ow->mode = OW_MODE_COMMAND;
    ow->speed = SPEEDSEL_FLEX;
    if (ser_open(&ow->ser, port) != 0)
        return 0;
    if (detect_with_recovery(ow) && ow_read_version(ow, &v) == 0 && v.crc_ok && v.major >= 1) {
        ow->ver = v;
        return 1;
    }
    ser_close(&ow->ser);
    return 0;
}

int ow_open(ow_t *ow, const char *port, int debug, char *found_port, size_t found_size)
{
    char name[16];
    int i;

    if (port) {
        if (!try_port(ow, port, debug))
            return -1;
        snprintf(found_port, found_size, "%s", port);
        return 0;
    }
    for (i = 1; i <= 256; i++) {  /* same scan as the Maxim reference */
        snprintf(name, sizeof name, "COM%d", i);
        if (try_port(ow, name, debug)) {
            snprintf(found_port, found_size, "%s", name);
            return 0;
        }
    }
    return -1;
}

void ow_close(ow_t *ow)
{
    ser_close(&ow->ser);
}
