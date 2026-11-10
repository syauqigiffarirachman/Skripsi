#include "serial_win32.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

static void trace(const serial_t *s, char dir, const uint8_t *d, size_t n)
{
    size_t i;
    if (!s->debug)
        return;
    fputc(dir, stderr);
    for (i = 0; i < n; i++)
        fprintf(stderr, " %02X", d[i]);
    fputc('\n', stderr);
}

void ser_sleep(int ms)
{
    Sleep((DWORD)ms);
}

int ser_open(serial_t *s, const char *port)
{
    char path[64];
    HANDLE h;
    DCB dcb;

    s->h = NULL;
    snprintf(path, sizeof path, "\\\\.\\%s", port);
    h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    SetupComm(h, 2048, 2048);
    memset(&dcb, 0, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        return -1;
    }
    dcb.BaudRate = CBR_9600;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;   /* DTR on  = adapter out of reset */
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;  /* RTS off = no 7V/9V pulse */
    dcb.fAbortOnError = FALSE;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        return -1;
    }
    PurgeComm(h, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    s->h = h;
    return 0;
}

void ser_close(serial_t *s)
{
    if (s->h) {
        EscapeCommFunction((HANDLE)s->h, CLRDTR);
        CloseHandle((HANDLE)s->h);
        s->h = NULL;
    }
}

void ser_flush(serial_t *s)
{
    PurgeComm((HANDLE)s->h, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}

int ser_write(serial_t *s, const uint8_t *d, size_t n)
{
    DWORD written = 0;
    COMMTIMEOUTS ct;

    trace(s, '>', d, n);
    memset(&ct, 0, sizeof ct);
    ct.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts((HANDLE)s->h, &ct);
    if (!WriteFile((HANDLE)s->h, d, (DWORD)n, &written, NULL))
        return 0;
    return written == (DWORD)n;
}

size_t ser_read(serial_t *s, uint8_t *d, size_t n)
{
    DWORD got = 0;
    COMMTIMEOUTS ct;

    memset(&ct, 0, sizeof ct);
    ct.ReadTotalTimeoutConstant = (DWORD)(40 * n + 205);  /* returns early once n bytes arrive */
    ct.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts((HANDLE)s->h, &ct);
    if (!ReadFile((HANDLE)s->h, d, (DWORD)n, &got, NULL))
        got = 0;
    trace(s, '<', d, got);
    return got;
}

void ser_break(serial_t *s, int ms)
{
    SetCommBreak((HANDLE)s->h);
    Sleep((DWORD)ms);
    ClearCommBreak((HANDLE)s->h);
}

void ser_set_baud(serial_t *s, uint8_t code)
{
    DCB dcb;
    memset(&dcb, 0, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    GetCommState((HANDLE)s->h, &dcb);
    switch (code) {
    case DS_BAUD_115200: dcb.BaudRate = CBR_115200; break;
    case DS_BAUD_57600:  dcb.BaudRate = CBR_57600;  break;
    case DS_BAUD_19200:  dcb.BaudRate = CBR_19200;  break;
    default:          dcb.BaudRate = CBR_9600;   break;
    }
    SetCommState((HANDLE)s->h, &dcb);
}

void ser_set_dtr(serial_t *s, int state)
{
    EscapeCommFunction((HANDLE)s->h, state ? SETDTR : CLRDTR);
}
