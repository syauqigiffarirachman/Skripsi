"""I2C master over the DS9481P-300 (in I2C mode) / DS9400.

Port of Maxim's i2c_ds9481p_ds9400.c. The adapter takes single ASCII
characters as commands:
    'S' start   'T' repeated start   'P' stop
    'Q' + byte  write byte and report ACK (0x00 = ACK)   'W' + byte  write byte
    'R' read byte + ACK   'N' read byte + NACK
    'H' hello/status      'V' adapter version       'M' + 'F'/'S'  fast/standard
The serial link runs at 115200 baud in this mode.
"""

from .errors import CommError

I2C_WRITE = 0
I2C_READ = 1
BAUD_115200 = 0x06


class I2C:
    def __init__(self, transport):
        self.tp = transport
        self._check_write = False

    # ------------------------------------------------------------------
    # adapter housekeeping
    # ------------------------------------------------------------------
    def connect(self, settle_ms=4000):
        """Say hello to the adapter. True if it answered.

        Maxim waits 4 s first because the adapter's microcontroller needs
        time to start after the port is opened / reset.
        """
        self.tp.sleep(settle_ms)
        self.stop()  # in case a transfer was left open
        self.tp.flush()
        if self.tp.write(b"H") and len(self.tp.read(1)) == 1:
            return True
        return False

    def read_version(self):
        """Adapter firmware version byte (e.g. 0x05 = 0.5), or None."""
        self.tp.flush()
        if self.tp.write(b"V"):
            rx = self.tp.read(1)
            if len(rx) == 1:
                return rx[0]
        self.adapter_reset()
        return None

    def adapter_reset(self):
        """Reset the adapter's microcontroller via RTS/DTR (takes ~5.5 s)."""
        self.tp.set_rts(False)
        self.tp.set_dtr(False)
        self.tp.sleep(1500)
        self.tp.set_dtr(True)
        self.tp.set_rts(True)
        self.tp.sleep(4000)

    def set_speed(self, fast):
        self.tp.write(b"MF" if fast else b"MS")

    def switch_to_onewire(self):
        """Ask the DS9481P-300 to go back to 1-Wire mode ('C' then 'O')."""
        self.tp.write(b"C")
        ok = self.tp.write(b"O")
        self.tp.flush()
        return ok

    # ------------------------------------------------------------------
    # primitives
    # ------------------------------------------------------------------
    def start(self):
        self.tp.write(b"S")
        self._check_write = True

    def stop(self):
        self.tp.write(b"P")

    def write_byte(self, value, expect_ack=True):
        """Write one byte. The first byte after Start (the address) is
        verified with 'Q'; later bytes use plain 'W'. Returns False on NACK."""
        if self._check_write:
            self.tp.flush()
            self.tp.write(bytes([ord("Q"), value]))
            self._check_write = False
            rx = self.tp.read(1)
            if len(rx) != 1:
                self.adapter_reset()
                return False
            return rx[0] == 0
        self.tp.write(bytes([ord("W"), value]))
        return True

    def read_byte(self, ack):
        self.tp.flush()
        self.tp.write(b"R" if ack else b"N")
        rx = self.tp.read(1)
        if len(rx) != 1:
            self.adapter_reset()
            raise CommError("I2C read: no byte from adapter")
        return rx[0]

    # ------------------------------------------------------------------
    # Linux-style helpers
    # ------------------------------------------------------------------
    def write(self, addr, data):
        """Start, addr|W, data..., Stop. Returns True when every byte was sent
        and the address was ACKed. `addr` is the 8-bit form (e.g. 0x76)."""
        self.start()
        if not self.write_byte((addr & 0xFE) | I2C_WRITE):
            self.stop()
            return False
        for b in data:
            self.write_byte(b)
        self.stop()
        return True

    def read(self, addr, count):
        """Start, addr|R, read `count` bytes (NACK on the last), Stop.
        Returns bytes, or None if the address was not ACKed."""
        self.start()
        if not self.write_byte((addr & 0xFE) | I2C_READ):
            self.stop()
            return None
        out = bytearray()
        try:
            for i in range(count):
                out.append(self.read_byte(ack=(i != count - 1)))
        except CommError:
            self.stop()
            return None
        self.stop()
        return bytes(out)
