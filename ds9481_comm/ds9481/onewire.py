"""1-Wire master over the DS9481 (DS2480B-style serial protocol).

Port of the essential parts of Maxim's 1wire_ds9481.c. The adapter has two
serial "modes":
  * COMMAND mode (0xE3): bytes are adapter commands (reset, one bit, config)
  * DATA mode    (0xE1): bytes are sent straight onto the 1-Wire bus and the
                         bus value is echoed back. A data byte equal to 0xE3
                         must be doubled so it is not read as a mode switch.
"""

from .crc import crc8
from .errors import CommError

# --- mode switch bytes
MODE_DATA = 0xE1
MODE_COMMAND = 0xE3
MODE_STOP_PULSE = 0xF1

# --- command byte building blocks
CMD_COMM = 0x81
CMD_CONFIG = 0x01
FUNCTSEL_BIT = 0x00
FUNCTSEL_RESET = 0x40
FUNCTSEL_CHMOD = 0x60
BITPOL_ONE = 0x10
BITPOL_ZERO = 0x00
SPEEDSEL_FLEX = 0x04
SPEEDSEL_PULSE = 0x0C
PRIME5V_TRUE = 0x02

PARMSEL_PARMREAD = 0x00
PARMSEL_VERSION_EX = 0x00
PARMSEL_SLEW = 0x10
PARMSEL_5VPULSE = 0x30
PARMSEL_WRITE1LOW = 0x40
PARMSEL_SAMPLEOFFSET = 0x50
PARMSEL_BAUDRATE = 0x70
PARMSET_infinite = 0x0E
PARMSET_Slew1p37Vus = 0x06
PARMSET_Write10us = 0x04
PARMSET_SampOff8us = 0x0A

RB_RESET_MASK = 0x03
RB_PRESENCE = 0x01
RB_ALARMPRESENCE = 0x02

LEVEL_NORMAL = 0
LEVEL_STRONG5 = 2

CMD_SWITCH_I2C = 0xE5  # DS9481P-300: leave 1-Wire, enter I2C mode

MAX_BLOCK = 160  # largest OWBlock in the Maxim driver


class AdapterVersion:
    """DS9481 extended version record, laid out like Maxim's ver[] array."""

    def __init__(self, raw, crc_ok):
        self.raw = bytes(raw)
        self.crc_ok = crc_ok

    @property
    def firmware(self):  # (major, minor); Maxim requires >= 1.0
        v = self.raw[3] if len(self.raw) > 3 else 0
        return (v >> 4, v & 0x0F)

    @property
    def i2c_supported(self):
        return len(self.raw) > 11 and (self.raw[11] & 0x10) == 0x10

    @property
    def valid(self):
        return len(self.raw) > 4 and self.crc_ok and self.firmware >= (1, 0)


class OneWire:
    def __init__(self, transport):
        self.tp = transport
        self.mode = MODE_COMMAND     # which serial mode the adapter is in
        self.level = LEVEL_NORMAL    # normal / 5V strong pull-up
        self.speed = SPEEDSEL_FLEX   # standard ("flex") 1-Wire speed
        self.baud = 0x00             # 9600

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------
    def _need(self, pkt, mode):
        """Append the mode-switch byte to pkt if the adapter is not in `mode`."""
        if self.mode != mode:
            self.mode = mode
            pkt.append(mode)

    def _txrx(self, pkt, nread, what):
        """flush, send pkt, read nread bytes. Re-syncs the adapter on failure."""
        self.tp.flush()
        if self.tp.write(pkt):
            rx = self.tp.read(nread) if nread else b""
            if len(rx) == nread:
                return rx
        self.detect()  # error: re-sync with the DS9481 (same as Maxim driver)
        raise CommError("DS9481 %s: no/short response" % what)

    # ------------------------------------------------------------------
    # adapter detection / info
    # ------------------------------------------------------------------
    def detect(self):
        """Reset the adapter's UART state and set the FLEX parameters.

        Returns True when a DS9481 answered correctly.
        """
        self.mode = MODE_COMMAND
        self.baud = 0x00
        self.speed = SPEEDSEL_FLEX
        self.level = LEVEL_NORMAL
        self.tp.set_baud(self.baud)
        self.tp.send_break(2)
        self.tp.sleep(2)
        self.tp.flush()
        if not self.tp.write(b"\xC1"):  # timing byte
            return False
        self.tp.sleep(2)

        pkt = bytes([
            CMD_CONFIG | PARMSEL_SLEW | PARMSET_Slew1p37Vus,          # PDSRC 1.37V/us
            CMD_CONFIG | PARMSEL_WRITE1LOW | PARMSET_Write10us,       # W1LT 10us
            CMD_CONFIG | PARMSEL_SAMPLEOFFSET | PARMSET_SampOff8us,   # DSO 8us
            CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3),  # read baud
            CMD_COMM | FUNCTSEL_BIT | self.baud | BITPOL_ONE,         # 1-bit self test
        ])
        self.tp.flush()
        if self.tp.write(pkt):
            rx = self.tp.read(5)
            if len(rx) == 5:
                return ((rx[3] & 0xF1) == 0x00 and (rx[3] & 0x0E) == self.baud
                        and (rx[4] & 0xF0) == 0x90 and (rx[4] & 0x0C) == self.baud)
        return False

    def read_version(self):
        """Read the DS9481 extended version string. Returns AdapterVersion or None."""
        pkt = []
        self._need(pkt, MODE_COMMAND)
        pkt.append(CMD_CONFIG | PARMSEL_PARMREAD | PARMSEL_VERSION_EX)
        self.tp.flush()
        if not self.tp.write(pkt):
            return None
        first = self.tp.read(1)
        if len(first) != 1 or first[0] == 0 or first[0] + 1 >= 60:
            return None
        ln = first[0]
        rest = self.tp.read(ln + 1)
        if len(rest) != ln + 1:
            return None
        raw = first + rest
        return AdapterVersion(raw, crc8(raw) == 0)

    def switch_to_i2c(self):
        """Tell the DS9481P-300 to leave 1-Wire mode and become an I2C adapter.

        After this call this OneWire object is no longer usable; reopen the
        port at 115200 baud and talk with the I2C class.
        """
        pkt = []
        self._need(pkt, MODE_COMMAND)
        pkt.append(CMD_SWITCH_I2C)
        ok = self.tp.write(pkt)
        self.tp.flush()  # the adapter sends no reply
        return ok

    # ------------------------------------------------------------------
    # bus level (normal / strong pull-up)
    # ------------------------------------------------------------------
    def level_normal(self):
        """Release the 5V strong pull-up (no-op if already normal)."""
        if self.level == LEVEL_NORMAL:
            return
        pkt = []
        self._need(pkt, MODE_COMMAND)
        pkt.append(MODE_STOP_PULSE)
        pkt.append(CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE)  # pulse w/o prime
        pkt.append(MODE_STOP_PULSE)
        rx = self._txrx(pkt, 2, "stop pulse")
        if (rx[0] & 0xE0) == 0xE0 and (rx[1] & 0xE0) == 0xE0:
            self.level = LEVEL_NORMAL
        else:
            self.detect()
            raise CommError("DS9481 refused to drop the strong pull-up")

    def write_byte_power(self, value):
        """Write one byte, then leave the 5V strong pull-up on after the last bit.

        Returns True if the echoed byte equals `value`.
        """
        pkt = []
        self._need(pkt, MODE_COMMAND)
        pkt.append(CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite)  # SPUD time
        tmp = value & 0xFF
        for i in range(8):
            b = BITPOL_ONE if tmp & 1 else BITPOL_ZERO
            b |= CMD_COMM | FUNCTSEL_BIT | self.speed
            if i == 7:
                b |= PRIME5V_TRUE
            pkt.append(b)
            tmp >>= 1
        rx = self._txrx(pkt, 9, "write byte power")
        if (rx[0] & 0x81) != 0:
            return False
        self.level = LEVEL_STRONG5
        echo = 0
        for i in range(8):
            echo >>= 1
            if rx[i + 1] & 0x01:
                echo |= 0x80
        return echo == (value & 0xFF)

    # ------------------------------------------------------------------
    # basic 1-Wire operations
    # ------------------------------------------------------------------
    def reset(self):
        """1-Wire reset. True if at least one device answered with a presence pulse."""
        try:
            self.level_normal()
            pkt = []
            self._need(pkt, MODE_COMMAND)
            pkt.append(CMD_COMM | FUNCTSEL_RESET | self.speed)
            rx = self._txrx(pkt, 1, "reset")
        except CommError:
            return False
        return (rx[0] & RB_RESET_MASK) in (RB_PRESENCE, RB_ALARMPRESENCE)

    def touch_byte(self, value):
        """Send one byte (0xFF to read) and return the byte seen on the bus."""
        self.level_normal()
        pkt = []
        self._need(pkt, MODE_DATA)
        pkt.append(value & 0xFF)
        if (value & 0xFF) == MODE_COMMAND:
            pkt.append(MODE_COMMAND)
        return self._txrx(pkt, 1, "touch byte")[0]

    def write_byte(self, value):
        self.touch_byte(value)

    def read_byte(self):
        return self.touch_byte(0xFF)

    def block(self, data):
        """Send `data` and return the bytes seen on the bus (same length).

        To read, send 0xFF placeholders. Max 160 bytes.
        """
        data = bytes(data)
        if len(data) > MAX_BLOCK:
            raise ValueError("block too long (%d > %d)" % (len(data), MAX_BLOCK))
        self.level_normal()
        pkt = []
        self._need(pkt, MODE_DATA)
        for b in data:
            pkt.append(b)
            if b == MODE_COMMAND:
                pkt.append(b)  # escape
        return self._txrx(pkt, len(data), "block")

    # ------------------------------------------------------------------
    # ROM commands
    # ------------------------------------------------------------------
    def skip_rom(self):
        if self.reset():
            self.write_byte(0xCC)
            return True
        return False

    def resume(self):
        if self.reset():
            self.write_byte(0xA5)
            return True
        return False

    def match_rom(self, rom):
        rom = bytes(rom)
        if len(rom) != 8:
            raise ValueError("ROM number must be 8 bytes")
        if self.reset():
            self.block(b"\x55" + rom)
            return True
        return False

    def read_rom(self):
        """READ ROM (0x33). Only valid with ONE device on the bus.

        Returns the 8-byte ROM number, or None if absent / CRC8 wrong.
        """
        if not self.reset():
            return None
        rx = self.block(b"\x33" + b"\xFF" * 8)
        rom = rx[1:9]
        if crc8(rom) == 0 and rom[0] != 0:
            return rom
        return None
