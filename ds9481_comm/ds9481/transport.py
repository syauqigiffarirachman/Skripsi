"""Serial (virtual COM port) transport for the DS9481P-300 USB adapter.

The adapter shows up in Windows as a COM port. Behaviour copied from the
Maxim reference (serial_win32ex.c):
  * 8N1, starts at 9600 baud
  * DTR asserted (adapter powered / out of reset), RTS de-asserted
  * read timeout = 40 ms per byte + 205 ms
pyserial is imported lazily so the rest of the package can be unit tested
without it.
"""

import sys
import time

# Baud-rate codes used by the DS9481 protocol -> real baud rate
BAUD_CODES = {0x00: 9600, 0x02: 19200, 0x04: 57600, 0x06: 115200}


class SerialTransport:
    def __init__(self, port, debug=False):
        self.port = port
        self.debug = debug
        self._s = None

    # ---- lifecycle -------------------------------------------------------
    def open(self):
        import serial  # lazy: pip install pyserial

        s = serial.Serial()
        s.port = self.port
        s.baudrate = 9600
        s.bytesize = serial.EIGHTBITS
        s.parity = serial.PARITY_NONE
        s.stopbits = serial.STOPBITS_ONE
        s.xonxoff = False
        s.rtscts = False
        s.dsrdtr = False
        s.dtr = True
        s.rts = False
        s.timeout = 0.25
        s.write_timeout = 1.0
        s.open()  # raises serial.SerialException if the port is busy / absent
        s.reset_input_buffer()
        s.reset_output_buffer()
        self._s = s

    def close(self):
        if self._s is not None:
            try:
                self._s.dtr = False
            except Exception:
                pass
            try:
                self._s.close()
            finally:
                self._s = None

    # ---- byte I/O ---------------------------------------------------------
    def flush(self):
        self._s.reset_input_buffer()
        self._s.reset_output_buffer()

    def write(self, data):
        data = bytes(data)
        if self.debug:
            self._log(">", data)
        try:
            n = self._s.write(data)
            self._s.flush()
        except Exception:
            return False
        return n == len(data)

    def read(self, n):
        """Read up to n bytes (Maxim timeout 40ms*n + 205ms). May return fewer."""
        self._s.timeout = (40 * n + 205) / 1000.0
        data = self._s.read(n)
        if self.debug:
            self._log("<", data)
        return bytes(data)

    # ---- line control ------------------------------------------------------
    def send_break(self, ms=2):
        self._s.break_condition = True
        time.sleep(ms / 1000.0)
        self._s.break_condition = False

    def set_baud(self, baud_code):
        self._s.baudrate = BAUD_CODES.get(baud_code, 9600)

    def set_baud_rate(self, baud):
        self._s.baudrate = baud

    def set_dtr(self, state):
        self._s.dtr = bool(state)

    def set_rts(self, state):
        self._s.rts = bool(state)

    def sleep(self, ms):
        time.sleep(ms / 1000.0)

    # ---- debug --------------------------------------------------------------
    def _log(self, arrow, data):
        sys.stderr.write("%s %s\n" % (arrow, " ".join("%02X" % b for b in data)))


def list_serial_ports():
    """Names of serial ports present on this machine (e.g. ['COM3', 'COM7'])."""
    from serial.tools import list_ports

    return [p.device for p in list_ports.comports()]
