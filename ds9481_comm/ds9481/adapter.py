"""DS9481P-300 adapter manager: finds the COM port and switches between the
1-Wire personality (for the DS28E38) and the I2C personality (for the DS2476).

The adapter is only ever in ONE mode. `adapter.onewire()` and `adapter.i2c()`
switch automatically when needed, so the chip classes can simply ask for the
bus they need.
"""

import time

from .errors import AdapterNotFound
from .i2c import BAUD_115200, I2C
from .onewire import OneWire
from .transport import SerialTransport, list_serial_ports

MODE_NONE, MODE_ONEWIRE, MODE_I2C = "none", "1-wire", "i2c"


class DS9481:
    def __init__(self, port=None, debug=False, transport_factory=None,
                 i2c_settle_ms=4000, reenumerate_ms=1000):
        """
        port            COM port (e.g. "COM5"). None = scan all serial ports.
        debug           print every byte sent (>) / received (<) to stderr.
        transport_factory  callable(port) -> transport (used by the tests).
        """
        self.port = port
        self.debug = debug
        self._factory = transport_factory or (lambda p: SerialTransport(p, debug))
        self._list_ports = list_serial_ports
        self.i2c_settle_ms = i2c_settle_ms
        self.reenumerate_ms = reenumerate_ms
        self.mode = MODE_NONE
        self.tp = None
        self._ow = None
        self._i2c = None
        self.version = None  # AdapterVersion (1-Wire mode)

    # ---- context manager ---------------------------------------------------
    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def close(self):
        if self.mode == MODE_I2C and self._i2c is not None:
            try:                       # leave the adapter in its default 1-Wire mode
                self._i2c.switch_to_onewire()
                self.tp.sleep(200)
            except Exception:
                pass
        if self.tp is not None:
            self.tp.close()
        self.tp = self._ow = self._i2c = None
        self.mode = MODE_NONE

    # ---- candidate ports -----------------------------------------------------
    def _candidates(self):
        ports = []
        if self.port:
            ports.append(self.port)
        try:
            for p in self._list_ports():
                if p not in ports:
                    ports.append(p)
        except ImportError:
            raise                      # pyserial missing: say so, don't hide it
        except Exception:
            pass
        return ports

    # ---- 1-Wire personality -----------------------------------------------------
    def _open_onewire(self):
        for p in self._candidates():
            tp = self._factory(p)
            try:
                tp.open()
            except ImportError:
                raise
            except Exception:
                continue
            ow = self._detect_with_recovery(tp)
            if ow is not None:
                ver = ow.read_version()
                if ver is not None and ver.valid:
                    self.tp, self._ow, self._i2c = tp, ow, None
                    self.port, self.version, self.mode = p, ver, MODE_ONEWIRE
                    return ow
            tp.close()
        tried = ", ".join(self._candidates()) or "none"
        raise AdapterNotFound("no DS9481 found (ports tried: %s)" % tried)

    @staticmethod
    def _detect_with_recovery(tp):
        """Detect the adapter in 1-Wire mode; if it does not answer, assume it is
        stuck in I2C mode from an earlier run and bring it back ('C','O'), then
        try a DTR power-on reset. Returns a OneWire or None."""
        ow = OneWire(tp)
        if ow.detect():
            return ow
        tp.set_baud(0x06)                      # I2C mode talks at 115200
        tp.write(b"C")
        tp.write(b"O")
        tp.flush()
        tp.sleep(500)
        tp.set_baud(0x00)
        ow = OneWire(tp)
        if ow.detect():
            return ow
        tp.set_dtr(False)                      # power-on reset (Maxim DS9481_POR)
        tp.sleep(500)
        tp.set_dtr(True)
        tp.sleep(500)
        ow = OneWire(tp)
        return ow if ow.detect() else None

    def onewire(self):
        """Return a ready OneWire bus object (switching from I2C if needed)."""
        if self.mode == MODE_ONEWIRE:
            return self._ow
        if self.mode == MODE_I2C:
            return self._i2c_to_onewire()
        return self._open_onewire()

    # ---- I2C personality ------------------------------------------------------------
    def i2c(self):
        """Return a ready I2C bus object (switching from 1-Wire if needed)."""
        if self.mode == MODE_I2C:
            return self._i2c
        if self.mode == MODE_NONE:
            self._open_onewire()
        if not self.version.i2c_supported:
            raise AdapterNotFound("this DS9481 firmware does not support I2C")
        return self._onewire_to_i2c()

    def _onewire_to_i2c(self):
        self._ow.switch_to_i2c()
        first = self.port
        self.tp.close()
        self.tp = self._ow = None
        self.mode = MODE_NONE
        # Maxim closes the port and re-scans: reopen the same port first.
        order = [first] + [p for p in self._candidates() if p != first]
        time.sleep(self.reenumerate_ms / 1000.0)  # let USB / adapter settle
        for p in order:
            tp = self._factory(p)
            try:
                tp.open()
            except ImportError:
                raise
            except Exception:
                continue
            tp.set_baud(BAUD_115200)
            bus = I2C(tp)
            if bus.connect(self.i2c_settle_ms):
                self.tp, self._i2c, self.port, self.mode = tp, bus, p, MODE_I2C
                return bus
            tp.close()
        raise AdapterNotFound("DS9481 did not answer in I2C mode")

    def _i2c_to_onewire(self):
        self._i2c.switch_to_onewire()
        self.tp.sleep(self.reenumerate_ms)
        # same port first, back at 9600 baud
        self.tp.set_baud(0x00)
        ow = OneWire(self.tp)
        if ow.detect():
            self._ow, self._i2c, self.mode = ow, None, MODE_ONEWIRE
            return ow
        # otherwise re-scan from scratch
        self.tp.close()
        self.tp = self._i2c = None
        self.mode = MODE_NONE
        return self._open_onewire()
