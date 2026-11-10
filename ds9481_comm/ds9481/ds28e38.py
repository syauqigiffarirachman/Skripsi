"""DS28E38 (ECDSA secure authenticator, 1-Wire) command layer.

Every command uses the same "XPC" frame (ported from ds28e38.c):

  reset + ROM command
  TX  0x66 | length | sub-command | parameters ...  -> RX inverted CRC16 (2 B)
  TX  release byte 0xAA with 5 V strong pull-up for the processing time
  RX  dummy | length | result | data ... | inverted CRC16 (2 B)

Result byte 0xAA = success.
"""

from collections import namedtuple

from .crc import CRC16_RESIDUE, crc16
from .errors import CommError, DeviceError

XPC_COMMAND = 0x66
CMD_WRITE_MEM = 0x96
CMD_READ_MEM = 0x44      # value from Maxim's ds28e38.h (not 0x69)
CMD_READ_STATUS = 0xAA
CMD_SET_PAGE_PROT = 0xC3
CMD_COMP_READ_AUTH = 0xA5
CMD_DECREMENT_CNT = 0xC9
CMD_GEN_ECDSA_KEY = 0xCB
CMD_READ_RNG = 0xD2

RESULT_SUCCESS = 0xAA

# protection bits
PROT_RP, PROT_WP, PROT_EM, PROT_DC, PROT_PF = 0x01, 0x02, 0x04, 0x08, 0x10
ECDSA_KEY_LOCK = 0x80
ECDSA_USE_PUF = 0x01

# processing delays (ms), 128x PUF oversampling values from ds28e38.h
DELAY_WRITE = 180
DELAY_READ = 120
DELAY_ECDSA = 520
DELAY_TCMP = 25

PG_DEC_COUNTER = 3
PG_PUB_KEY_X, PG_PUB_KEY_Y, PG_PRIV_KEY = 4, 5, 6

Status = namedtuple("Status", "protection manid flavor extra")


class DS28E38:
    """
    bus     a DS9481 adapter object (recommended: the chip then switches the
            adapter to 1-Wire by itself) or a plain OneWire instance.
    select  ROM selection used before every command: "skip" (one device on the
            bus), "match" (uses self.rom_no) or "resume".
    """

    def __init__(self, bus, select="skip"):
        self._bus = bus
        self.select = select
        self.rom_no = None
        self.manid = None
        self.last_result = None

    # ---- bus access --------------------------------------------------------
    def _ow(self):
        return self._bus.onewire() if hasattr(self._bus, "onewire") else self._bus

    def _select(self, ow):
        if self.select == "skip":
            ok = ow.skip_rom()
        elif self.select == "match":
            if self.rom_no is None:
                raise ValueError("select='match' needs rom_no (run detect() first)")
            ok = ow.match_rom(self.rom_no)
        elif self.select == "resume":
            ok = ow.resume()
        else:
            raise ValueError("unknown select method %r" % self.select)
        if not ok:
            raise CommError("DS28E38: no presence pulse on the 1-Wire bus")

    # ---- the standard command flow -------------------------------------------------
    def _command(self, sub, delay_ms, expect_len, what):
        """Run one XPC command. `sub` = sub-command + parameters.
        Returns the bytes after the result byte. Raises on any failure."""
        ow = self._ow()
        self._select(ow)
        self.last_result = None

        frame = bytes([XPC_COMMAND, len(sub)]) + bytes(sub) + b"\xFF\xFF"
        rx = ow.block(frame)
        if crc16(rx) != CRC16_RESIDUE:
            raise CommError("%s: CRC16 of the command echo is wrong" % what)

        ow.write_byte_power(0xAA)  # release byte + strong pull-up
        if delay_ms:
            ow.tp.sleep(delay_ms)
        ow.level_normal()

        hdr = ow.block(b"\xFF\xFF")  # dummy byte, length byte
        n = hdr[1]
        if n == 0xFF or n == 0:
            raise CommError("%s: invalid response length 0x%02X" % (what, n))
        body = ow.block(b"\xFF" * (n + 2))
        if crc16(bytes([n]) + body) != CRC16_RESIDUE:
            raise CommError("%s: CRC16 of the response is wrong" % what)

        result = body[0]
        self.last_result = result
        if result != RESULT_SUCCESS:
            raise DeviceError(result, what)
        if n != expect_len:
            raise CommError("%s: expected length %d, got %d" % (what, expect_len, n))
        return bytes(body[1:n])

    # ---- commands ---------------------------------------------------------------------
    def read_status(self, rng_test=False):
        """Protection of pages 0-6, MANID and ROM version."""
        d = self._command([CMD_READ_STATUS, 1 if rng_test else 0],
                          DELAY_READ, 13, "read status")
        return Status(protection=list(d[0:7]), manid=d[7:9], flavor=d[9:11], extra=d[11:])

    def read_memory(self, page):
        """Read one 32-byte page (pages 0-6; page 6 is read-protected)."""
        return self._command([CMD_READ_MEM, page & 0x07], DELAY_READ, 33, "read memory")

    def write_memory(self, page, data):
        data = bytes(data)
        if len(data) != 32:
            raise ValueError("a page is exactly 32 bytes")
        self._command([CMD_WRITE_MEM, page & 0x07] + list(data), DELAY_WRITE, 1,
                      "write memory")

    def read_rng(self, n):
        """n random bytes (1..64) from the on-chip generator."""
        if not 1 <= n <= 64:
            raise ValueError("n must be 1..64")
        # the chip is asked for (n-1) and returns n bytes after the result byte
        return self._command([CMD_READ_RNG, n - 1], DELAY_TCMP, n + 1, "read RNG")

    def decrement_counter(self):
        """Decrement the counter on page 3 (page needs DC protection set)."""
        self._command([CMD_DECREMENT_CNT], DELAY_WRITE, 1, "decrement counter")

    def compute_read_page_auth(self, page, challenge, anonymous=False):
        """ECDSA signature over (ROM | page | challenge | page# | MANID).

        Returns (r, s), 32 bytes each. The chip sends s first, then r.
        """
        challenge = bytes(challenge)
        if len(challenge) != 32:
            raise ValueError("challenge is exactly 32 bytes")
        param = (page & 0x07) | (0xE0 if anonymous else 0)
        d = self._command([CMD_COMP_READ_AUTH, param] + list(challenge),
                          DELAY_ECDSA, 65, "compute page authentication")
        s, r = d[:32], d[32:64]
        return r, s

    # ---- PERMANENT operations (explicit opt-in) ----------------------------------------------
    def set_page_protection(self, page, prot, i_understand_permanent=False):
        """Set page protection bits. Most bits (WP, RP, DC ...) can NEVER be undone."""
        if not i_understand_permanent:
            raise PermissionError("set_page_protection is irreversible: pass "
                                  "i_understand_permanent=True to proceed")
        self._command([CMD_SET_PAGE_PROT, page & 0x07, prot], DELAY_WRITE, 1,
                      "set page protection")

    def generate_key_pair(self, use_puf=False, lock=False, i_understand_permanent=False):
        """Generate the ECDSA key pair on the chip (lock=True makes it permanent)."""
        if not i_understand_permanent:
            raise PermissionError("generate_key_pair changes the device keys: pass "
                                  "i_understand_permanent=True to proceed")
        param = (ECDSA_KEY_LOCK if lock else 0) | (ECDSA_USE_PUF if use_puf else 0)
        self._command([CMD_GEN_ECDSA_KEY, param], DELAY_ECDSA, 1, "generate key pair")

    # ---- convenience ------------------------------------------------------------------------------
    def detect(self):
        """Read status (proves the chip answers) and fetch ROM number + MANID."""
        status = self.read_status()
        self.manid = bytes(status.manid)
        self.rom_no = self._ow().read_rom()
        return status

    def signed_message(self, page, page_data, challenge, anonymous=False):
        """The message the chip signed, to be hashed (SHA-256) and verified with
        the chip's public key: ROM | page data | challenge | page# | MANID."""
        rom = b"\xFF" * 8 if anonymous else self.rom_no
        if rom is None or self.manid is None:
            raise ValueError("run detect() first (need ROM number and MANID)")
        return bytes(rom) + bytes(page_data) + bytes(challenge) + bytes([page]) + self.manid
