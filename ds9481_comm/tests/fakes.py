"""A software model of a DS9481P-300 with a DS28E38 (1-Wire) and a DS2476 (I2C)
attached. It speaks the same byte protocol as the real adapter, so the whole
driver stack can be exercised without hardware.

NOTE: this is MY reading of the Maxim reference code, not captured traffic from
a real adapter. Passing these tests proves the frames/CRCs are self-consistent
with the Maxim C code, not that real silicon accepts them.
"""

import os

from ds9481.crc import crc8, crc16

MODE_DATA, MODE_COMMAND = 0xE1, 0xE3


def make_rom(family, serial6):
    body = bytes([family]) + bytes(serial6)
    return body + bytes([crc8(body)])


class FakeBus:
    """Shared state: lives across port close/re-open, like the real hardware."""

    def __init__(self):
        self.personality = "1w"          # "1w" or "i2c"
        # --- 1-Wire side
        self.ow_mode = "cmd"
        self.pulse_active = False
        self.bus_state = "idle"
        self.xpc = []
        self.release_bits = 0
        self.resp = []
        self.rom_idx = 0
        self.match_buf = []
        self.rom = make_rom(0x44, [1, 2, 3, 4, 5, 6])
        self.e38_manid = b"\x12\x34"
        self.e38_pages = [bytes([0x10 + p] * 32) for p in range(8)]
        self.e38_protection = [0, 0, 0, 0x08, 0, 0, 0x03]
        # --- I2C side
        self.i2c_pending = bytearray()
        self.i2c_dir = None
        self.i2c_resp = bytearray()
        self.d2476_rom = make_rom(0x4E, [9, 8, 7, 6, 5, 4])
        self.d2476_pages = [bytes([0x40 + p] * 32) for p in range(32)]
        po = bytearray(32)
        po[0] = 0x00
        po[22:24] = b"\xAB\xCD"
        po[24:32] = self.d2476_rom
        self.d2476_pages[28] = bytes(po)
        self.d2476_buffer = b""
        # --- bookkeeping for assertions
        self.log = []

    # ================= 1-Wire personality =================================
    def ow_reset_state(self):
        self.ow_mode = "cmd"
        self.pulse_active = False
        self.bus_state = "idle"

    def ow_feed(self, data, out):
        data = bytes(data)
        i = 0
        while i < len(data):
            b = data[i]
            i += 1
            if self.ow_mode == "cmd":
                self._ow_cmd_byte(b, out)
            else:
                if b == MODE_COMMAND:
                    if i < len(data) and data[i] == MODE_COMMAND:
                        i += 1                      # escaped literal 0xE3
                        self._ow_data_byte(b, out)
                    else:
                        self.ow_mode = "cmd"
                else:
                    self._ow_data_byte(b, out)

    def _ow_cmd_byte(self, b, out):
        if b == MODE_DATA:
            self.ow_mode = "data"
        elif b == MODE_COMMAND:
            pass
        elif b == 0xC1:
            pass                                    # timing byte, no reply
        elif b == 0xE5:                             # -> I2C personality
            self.personality = "i2c"
            self.i2c_pending = bytearray()
            self.i2c_resp = bytearray()
        elif b == 0xF1:                             # pulse start/stop
            if self.pulse_active:
                out.append(0xE0)
                self.pulse_active = False
            else:
                self.pulse_active = True
        elif b & 0x81 == 0x01:                      # configuration command
            if b == 0x0F:                           # read baud rate parameter
                out.append(0x00)
            elif b == 0x01:                         # read extended version
                out.extend(self._version_record())
            else:
                out.append(b & 0xFE)
        elif b & 0x81 == 0x81:                      # communication command
            func = b & 0x60
            if func == 0x40:                        # 1-Wire reset
                self.bus_state = "rom_cmd"
                out.append(0xCD)                    # presence pulse
            elif func == 0x60:                      # pulse (strong pull-up)
                out.append(b)
            elif func == 0x00:                      # single bit
                bit = (b >> 4) & 1
                out.append((b & 0xFC) | (3 if bit else 0))
                if self.bus_state == "await_release":
                    self.release_bits += 1
                    if self.release_bits == 8:
                        self.bus_state = "respond"
            else:
                out.append(b)

    def _version_record(self):
        ln = 12
        data = bytearray(ln)
        data[2] = 0x15                              # firmware 1.5
        data[10] = 0x10                             # I2C supported
        crc = crc8(bytes([ln]) + bytes(data))
        return bytes([ln]) + bytes(data) + bytes([crc])   # length, data, CRC8

    def _ow_data_byte(self, b, out):
        st = self.bus_state
        if st == "rom_cmd":
            if b == 0xCC:
                self.bus_state = "func"
                self.xpc = []
            elif b == 0xA5:
                self.bus_state = "func"
                self.xpc = []
            elif b == 0x33:
                self.bus_state = "readrom"
                self.rom_idx = 0
            elif b == 0x55:
                self.bus_state = "match"
                self.match_buf = []
            else:
                self.bus_state = "idle"
            out.append(b)
        elif st == "readrom":
            out.append(self.rom[self.rom_idx] & b)
            self.rom_idx += 1
            if self.rom_idx == 8:
                self.bus_state = "idle"
        elif st == "match":
            self.match_buf.append(b)
            out.append(b)
            if len(self.match_buf) == 8:
                self.bus_state = "func" if bytes(self.match_buf) == self.rom else "idle"
                self.xpc = []
        elif st == "func":
            self._xpc_byte(b, out)
        elif st == "respond":
            out.append(self.resp.pop(0) if self.resp else 0xFF)
            if not self.resp:
                self.bus_state = "idle"
        else:
            out.append(b)

    def _xpc_byte(self, b, out):
        idx = len(self.xpc)
        if idx == 0 and b != 0x66:
            out.append(b)
            return
        self.xpc.append(b)
        ln = self.xpc[1] if len(self.xpc) > 1 else None
        if ln is None or idx < ln + 2:
            out.append(b)                           # still the master's bytes
            return
        # the two trailing 0xFF: the chip returns the inverted CRC16
        crc = (~crc16(bytes(self.xpc[:ln + 2]))) & 0xFFFF
        word = [crc & 0xFF, crc >> 8]
        out.append(word[idx - (ln + 2)])
        if idx == ln + 3:
            self._xpc_execute(bytes(self.xpc[2:2 + ln]))

    def _xpc_execute(self, sub):
        self.log.append(("xpc", sub))
        cmd, par = sub[0], sub[1:]
        if cmd == 0xAA:                             # read status
            body = bytes([0xAA]) + bytes(self.e38_protection) + self.e38_manid + b"\x01\x02" + b"\x00"
        elif cmd == 0x44:                           # read memory
            body = bytes([0xAA]) + self.e38_pages[par[0] & 7]
        elif cmd == 0x96:                           # write memory
            self.e38_pages[par[0] & 7] = bytes(par[1:33])
            body = bytes([0xAA])
        elif cmd == 0xD2:                           # RNG
            body = bytes([0xAA]) + bytes((0x55 + i) & 0xFF for i in range(par[0] + 1))
        elif cmd == 0xA5:                           # compute page auth
            body = bytes([0xAA]) + b"\x11" * 32 + b"\x22" * 32
        elif cmd == 0xC9:                           # decrement counter
            body = bytes([0xAA])
        else:
            body = bytes([0x77])                    # parameter error
        n = len(body)
        crc = (~crc16(bytes([n]) + body)) & 0xFFFF
        self.resp = [0x00, n] + list(body) + [crc & 0xFF, crc >> 8]
        self.release_bits = 0
        self.bus_state = "await_release"

    # ================= I2C personality ======================================
    def i2c_feed(self, data, out):
        data = bytes(data)
        i = 0
        while i < len(data):
            c = chr(data[i])
            i += 1
            if c == "S":
                self.i2c_dir = None
                self.i2c_pending = bytearray()
            elif c in ("Q", "W"):
                v = data[i]
                i += 1
                if self.i2c_dir is None:            # first byte = address
                    ok = (v & 0xFE) == 0x76
                    self.i2c_dir = "r" if v & 1 else "w"
                    if c == "Q":
                        out.append(0x00 if ok else 0x01)
                else:
                    self.i2c_pending.append(v)
            elif c in ("R", "N"):
                out.append(self.i2c_resp.pop(0) if self.i2c_resp else 0xFF)
            elif c == "P":
                if self.i2c_dir == "w" and self.i2c_pending:
                    self._d2476_execute(bytes(self.i2c_pending))
                self.i2c_dir = None
                self.i2c_pending = bytearray()
            elif c == "H":
                out.append(0x00)
            elif c == "V":
                out.append(0x05)
            elif c == "M":
                i += 1
            elif c == "C":
                self.i2c_c_seen = True              # 'C' and 'O' arrive in separate writes
                continue
            elif c == "O" and getattr(self, "i2c_c_seen", False):
                self.personality = "1w"
                self.ow_reset_state()
                return
            self.i2c_c_seen = False

    def _d2476_execute(self, pkt):
        self.log.append(("i2c", pkt))
        cmd = pkt[0]
        ln = pkt[1] if len(pkt) > 1 else 0
        par = pkt[2:2 + ln]
        if cmd == 0x69:
            r = bytes([33, 0xAA]) + self.d2476_pages[par[0] & 0x1F]
        elif cmd == 0x96:
            self.d2476_pages[par[0] & 0x1F] = bytes(par[1:33])
            r = bytes([1, 0xAA])
        elif cmd == 0x87:
            self.d2476_buffer = bytes(par)
            r = b""
        elif cmd == 0xAA:
            r = bytes([1, 0x03])
        elif cmd == 0xD2:
            n = (par[0] & 0x3F) + 1
            r = bytes([n]) + bytes((0xA0 + i) & 0xFF for i in range(n))
        elif cmd == 0xA5:
            at = par[0] >> 5
            if at <= 2:
                r = bytes([33, 0xAA]) + b"\x77" * 32
            else:
                r = bytes([65, 0xAA]) + b"\x88" * 64
        else:
            r = bytes([1, 0x77])
        self.i2c_resp = bytearray(r)


class FakeTransport:
    """Transport interface used by OneWire / I2C, backed by a FakeBus."""

    def __init__(self, bus, port="COM_FAKE"):
        self.bus = bus
        self.port = port
        self.rx = bytearray()
        self.baud = 9600
        self.is_open = False
        self.sleeps = []

    def open(self):
        self.is_open = True

    def close(self):
        self.is_open = False

    def flush(self):
        self.rx.clear()

    def write(self, data):
        out = bytearray()
        if self.bus.personality == "1w":
            self.bus.ow_feed(data, out)
        else:
            self.bus.i2c_feed(data, out)
        self.rx.extend(out)
        return True

    def read(self, n):
        out = bytes(self.rx[:n])
        del self.rx[:n]
        return out

    def send_break(self, ms=2):
        if self.bus.personality == "1w":
            self.bus.ow_reset_state()
            self.rx.clear()

    def set_baud(self, code):
        self.baud = {0: 9600, 2: 19200, 4: 57600, 6: 115200}[code]

    def set_dtr(self, state):
        pass

    def set_rts(self, state):
        pass

    def sleep(self, ms):
        self.sleeps.append(ms)
