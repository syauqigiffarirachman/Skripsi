"""DS2476 (ECDSA / SHA-256 coprocessor, I2C) command layer.

Port of the command flow in ds2476.c:

  Start, addr|W, command, length, parameters..., Stop
  <delay>
  Start, addr|R, length, result/data..., Stop
"""

from .crc import crc8
from .errors import CommError, DeviceError

I2C_ADDR = 0x76          # 8-bit form (write address); 7-bit address is 0x3B

CMD_WRITE_MEM = 0x96
CMD_READ_MEM = 0x69
CMD_WRITE_BUF = 0x87
CMD_READ_PAGE_PROT = 0xAA
CMD_SET_PAGE_PROT = 0xC3
CMD_DECREMENT_CNT = 0xC9
CMD_READ_RNG = 0xD2
CMD_COMP_READ_AUTH = 0xA5

RESULT_SUCCESS = 0xAA

# delays (ms) from ds2476.h (already rounded up for PC timer resolution)
DELAY_EE_WRITE = 20
DELAY_EE_READ = 20
DELAY_COMPUTE = 20
DELAY_ECDSA_GEN = 50

PG_DECREMENT_CNT = 27
PG_ROM_OPTIONS = 28
PG_GPIO_CONTROL = 29

# Compute-and-read-authentication type (AT) field
AT_HMAC_SECRETA, AT_HMAC_SECRETB, AT_HMAC_SECRETS = 0, 1, 2
AT_ECDSA_KEYA, AT_ECDSA_KEYB, AT_ECDSA_KEYC = 3, 4, 5


class DS2476:
    """
    bus   a DS9481 adapter object (recommended: it switches the adapter to I2C
          by itself) or a plain I2C instance.
    """

    def __init__(self, bus, addr=I2C_ADDR):
        self._bus = bus
        self.addr = addr
        self.rom_no = None
        self.manid = None
        self.last_result = None

    def _i2c(self):
        return self._bus.i2c() if hasattr(self._bus, "i2c") else self._bus

    # ---- the standard command flow -------------------------------------------------
    def _command(self, cmd, params, delay_ms, expect_len, what):
        """Send a command and read back `expect_len` bytes (None = no read)."""
        bus = self._i2c()
        pkt = bytes([cmd]) + (bytes([len(params)]) + bytes(params) if params else b"")
        if not bus.write(self.addr, pkt):
            raise CommError("%s: DS2476 did not ACK the command" % what)
        if delay_ms:
            bus.tp.sleep(delay_ms)
        if expect_len is None:
            return b""
        rx = bus.read(self.addr, expect_len + 1)
        if rx is None:
            raise CommError("%s: DS2476 did not ACK the read (still busy?)" % what)
        if rx[0] != expect_len:
            raise CommError("%s: expected length %d, got %d" % (what, expect_len, rx[0]))
        return bytes(rx[1:])

    def _result_only(self, cmd, params, delay_ms, what):
        d = self._command(cmd, params, delay_ms, 1, what)
        self.last_result = d[0]
        if d[0] != RESULT_SUCCESS:
            raise DeviceError(d[0], what)

    # ---- commands ----------------------------------------------------------------------
    def read_memory(self, page):
        """Read a 32-byte page (pages 0-31; key/secret pages are read-protected)."""
        d = self._command(CMD_READ_MEM, [page & 0x1F], DELAY_EE_READ, 33, "read memory")
        self.last_result = d[0]
        if d[0] != RESULT_SUCCESS:
            raise DeviceError(d[0], "read memory")
        return d[1:]

    def write_memory(self, page, data):
        data = bytes(data)
        if len(data) != 32:
            raise ValueError("a page is exactly 32 bytes")
        self._result_only(CMD_WRITE_MEM, [page & 0x1F] + list(data),
                          DELAY_EE_WRITE, "write memory")

    def write_buffer(self, data):
        """Fill the chip's input buffer (1..80 bytes), e.g. with a challenge."""
        data = bytes(data)
        if not 1 <= len(data) <= 80:
            raise ValueError("buffer data is 1..80 bytes")
        bus = self._i2c()
        if not bus.write(self.addr, bytes([CMD_WRITE_BUF, len(data)]) + data):
            raise CommError("write buffer: DS2476 did not ACK")

    def read_page_protection(self, page):
        d = self._command(CMD_READ_PAGE_PROT, [page & 0x1F], DELAY_EE_READ, 1,
                          "read page protection")
        return d[0]

    def read_rng(self, n):
        """n random bytes (1..64); the chip is asked for n-1."""
        if not 1 <= n <= 64:
            raise ValueError("n must be 1..64")
        return self._command(CMD_READ_RNG, [(n - 1) & 0x3F], DELAY_COMPUTE, n,
                             "read RNG")

    def decrement_counter(self):
        self._result_only(CMD_DECREMENT_CNT, [], DELAY_EE_WRITE, "decrement counter")

    def compute_read_page_auth(self, page, auth_type):
        """HMAC (32 B) or ECDSA signature (64 B) of a page; the challenge must be
        loaded with write_buffer() first."""
        param = [((auth_type & 0x07) << 5) | (page & 0x1F)]
        if auth_type <= AT_HMAC_SECRETS:
            d = self._command(CMD_COMP_READ_AUTH, param,
                              DELAY_EE_READ + DELAY_COMPUTE, 33, "compute page auth")
        else:
            d = self._command(CMD_COMP_READ_AUTH, param,
                              DELAY_EE_READ + DELAY_ECDSA_GEN, 65, "compute page auth")
        self.last_result = d[0]
        if d[0] != RESULT_SUCCESS:
            raise DeviceError(d[0], "compute page auth")
        return d[1:]

    # ---- PERMANENT operation (explicit opt-in) -----------------------------------------------------
    def set_page_protection(self, page, prot, i_understand_permanent=False):
        if not i_understand_permanent:
            raise PermissionError("set_page_protection is irreversible: pass "
                                  "i_understand_permanent=True to proceed")
        self._result_only(CMD_SET_PAGE_PROT, [page & 0x1F, prot], DELAY_EE_WRITE,
                          "set page protection")

    # ---- convenience ------------------------------------------------------------------------------------
    def detect(self):
        """Read the ROM-options page and extract ROM number + MANID.
        Returns True when a DS2476 (family 0x4E) answers."""
        try:
            page = self.read_memory(PG_ROM_OPTIONS)
        except (CommError, DeviceError):
            return False
        rom = page[24:32]
        if crc8(rom) == 0 and (rom[0] & 0x7F) == 0x4E:
            self.rom_no, self.manid = bytes(rom), bytes(page[22:24])
            return True
        # ROM-block-read not enabled: Maxim accepts an all-FF ROM number
        if page[0] != 0xAA and page[24] == 0xFF:
            self.rom_no, self.manid = bytes(rom), bytes(page[22:24])
            return True
        return False
