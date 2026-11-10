"""Exception types for the DS9481 / DS28E38 / DS2476 communication package."""


class Ds9481Error(Exception):
    """Base class for every error raised by this package."""


class AdapterNotFound(Ds9481Error):
    """No DS9481 adapter answered on any serial port."""


class CommError(Ds9481Error):
    """Transport-level failure: timeout, bad CRC, no ACK, wrong length ..."""


class DeviceError(Ds9481Error):
    """The IC answered, but with a result byte other than 0xAA (success)."""

    NAMES = {
        0x55: "FAIL_PROTECTION",
        0x77: "FAIL_PARAMETER",
        0x33: "FAIL_INVALID_SEQUENCE",
        0x22: "FAIL_ECDSA",
        0x00: "FAIL_VERIFY",
        0x11: "FAIL_COMMUNICATION",
        0xFF: "FAIL_COMMUNICATION",
    }

    def __init__(self, code, what=""):
        self.code = code
        name = self.NAMES.get(code, "UNKNOWN")
        super().__init__("%s result byte 0x%02X (%s)" % (what or "command", code, name))
