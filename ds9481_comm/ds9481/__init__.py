"""Talk to the DS28E38 (1-Wire) and DS2476 (I2C) through a DS9481P-300 USB adapter."""

from .adapter import DS9481
from .crc import crc8, crc16
from .ds2476 import DS2476
from .ds28e38 import DS28E38
from .errors import AdapterNotFound, CommError, DeviceError, Ds9481Error
from .i2c import I2C
from .onewire import OneWire

__all__ = [
    "DS9481", "DS28E38", "DS2476", "OneWire", "I2C",
    "crc8", "crc16",
    "Ds9481Error", "AdapterNotFound", "CommError", "DeviceError",
]
