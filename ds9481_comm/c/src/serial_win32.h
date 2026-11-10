/* COM port (virtual serial port of the DS9481P-300) - Windows implementation.
 * Same behaviour as the Maxim reference / the Python transport:
 * 8N1, starts at 9600 baud, DTR asserted, RTS de-asserted,
 * read timeout = 40 ms per byte + 205 ms. */
#ifndef SERIAL_WIN32_H
#define SERIAL_WIN32_H

#include <stddef.h>
#include <stdint.h>

/* DS9481 baud-rate codes */
#define DS_BAUD_9600   0x00
#define DS_BAUD_19200  0x02
#define DS_BAUD_57600  0x04
#define DS_BAUD_115200 0x06

typedef struct {
    void *h;    /* HANDLE */
    int debug;  /* 1 = print every byte sent (>) / received (<) to stderr */
} serial_t;

int    ser_open(serial_t *s, const char *port);        /* "COM13"; 0 = ok, -1 = fail */
void   ser_close(serial_t *s);
void   ser_flush(serial_t *s);                          /* purge rx + tx buffers */
int    ser_write(serial_t *s, const uint8_t *d, size_t n);   /* 1 = all bytes written */
size_t ser_read(serial_t *s, uint8_t *d, size_t n);    /* bytes actually read (may be < n) */
void   ser_break(serial_t *s, int ms);
void   ser_set_baud(serial_t *s, uint8_t baud_code);
void   ser_set_dtr(serial_t *s, int state);
void   ser_sleep(int ms);

#endif
