"""Offline tests: run with  python -m unittest discover -s tests -v
(no hardware and no pyserial needed)."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from fakes import FakeBus, FakeTransport  # noqa: E402

from ds9481 import (DS2476, DS28E38, DS9481, CommError, DeviceError,  # noqa: E402
                    OneWire, crc8, crc16)
from ds9481.crc import CRC16_RESIDUE  # noqa: E402


def make_adapter():
    bus = FakeBus()
    ad = DS9481(port="COM_FAKE", transport_factory=lambda p: FakeTransport(bus, p),
                i2c_settle_ms=0, reenumerate_ms=0)
    ad._list_ports = lambda: ["COM_FAKE"]
    return ad, bus


class CrcTests(unittest.TestCase):
    def test_crc8_known_vector(self):
        # classic Dallas AN27 example: 02 1C B8 01 00 00 00 -> A2
        self.assertEqual(crc8(bytes([0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00])), 0xA2)

    def test_crc8_residue_zero(self):
        body = bytes([0x44, 1, 2, 3, 4, 5, 6])
        self.assertEqual(crc8(body + bytes([crc8(body)])), 0)

    def test_crc16_residue(self):
        data = bytes(range(40))
        inv = (~crc16(data)) & 0xFFFF
        frame = data + bytes([inv & 0xFF, inv >> 8])
        self.assertEqual(crc16(frame), CRC16_RESIDUE)


class OneWireTests(unittest.TestCase):
    def setUp(self):
        self.bus = FakeBus()
        self.tp = FakeTransport(self.bus)
        self.ow = OneWire(self.tp)

    def test_detect_and_version(self):
        self.assertTrue(self.ow.detect())
        ver = self.ow.read_version()
        self.assertTrue(ver.valid)
        self.assertEqual(ver.firmware, (1, 5))
        self.assertTrue(ver.i2c_supported)

    def test_reset_and_read_rom(self):
        self.ow.detect()
        self.assertTrue(self.ow.reset())
        self.assertEqual(self.ow.read_rom(), self.bus.rom)

    def test_block_escapes_0xE3(self):
        self.ow.detect()
        self.ow.reset()
        sent = []
        orig = self.tp.write
        self.tp.write = lambda d: (sent.append(bytes(d)), orig(d))[1]
        self.ow.block(bytes([0xE3, 0x01]))
        self.assertIn(bytes([0xE1, 0xE3, 0xE3, 0x01]), sent)

    def test_block_too_long(self):
        with self.assertRaises(ValueError):
            self.ow.block(b"\x00" * 161)


class DS28E38Tests(unittest.TestCase):
    def setUp(self):
        self.ad, self.bus = make_adapter()
        self.chip = DS28E38(self.ad)

    def tearDown(self):
        self.ad.close()

    def test_detect_reads_status_rom_manid(self):
        st = self.chip.detect()
        self.assertEqual(st.protection, self.bus.e38_protection)
        self.assertEqual(self.chip.manid, self.bus.e38_manid)
        self.assertEqual(self.chip.rom_no, self.bus.rom)

    def test_read_write_memory(self):
        self.chip.detect()
        self.assertEqual(self.chip.read_memory(2), bytes([0x12] * 32))
        self.chip.write_memory(2, bytes(range(32)))
        self.assertEqual(self.chip.read_memory(2), bytes(range(32)))

    def test_rng_length(self):
        self.assertEqual(len(self.chip.read_rng(16)), 16)
        self.assertEqual(len(self.chip.read_rng(1)), 1)

    def test_signature_split(self):
        r, s = self.chip.compute_read_page_auth(3, bytes(32))
        self.assertEqual((r, s), (b"\x22" * 32, b"\x11" * 32))  # chip sends s then r

    def test_match_rom_select(self):
        self.chip.detect()
        self.chip.select = "match"
        self.assertEqual(len(self.chip.read_memory(0)), 32)

    def test_device_error_surfaces_result_byte(self):
        ow = self.ad.onewire()
        self.chip.detect()
        with self.assertRaises(DeviceError) as cm:
            self.chip._command([0x01], 0, 1, "bogus")   # unknown sub-command
        self.assertEqual(cm.exception.code, 0x77)
        self.assertEqual(self.chip.last_result, 0x77)

    def test_permanent_ops_are_gated(self):
        with self.assertRaises(PermissionError):
            self.chip.set_page_protection(0, 0x02)
        with self.assertRaises(PermissionError):
            self.chip.generate_key_pair(lock=True)

    def test_signed_message_layout(self):
        self.chip.detect()
        msg = self.chip.signed_message(3, b"P" * 32, b"C" * 32)
        self.assertEqual(len(msg), 8 + 32 + 32 + 1 + 2)
        self.assertEqual(msg[:8], self.bus.rom)
        self.assertEqual(msg[-3], 3)
        anon = self.chip.signed_message(3, b"P" * 32, b"C" * 32, anonymous=True)
        self.assertEqual(anon[:8], b"\xFF" * 8)

    def test_bad_crc_detected(self):
        self.chip.detect()
        ow = self.ad.onewire()
        orig = ow.block
        calls = {"n": 0}

        def corrupt(data):
            rx = bytearray(orig(data))
            calls["n"] += 1
            if len(data) > 4 and data[0] == 0x66:
                rx[-1] ^= 0x01                      # flip a CRC bit
            return bytes(rx)

        ow.block = corrupt
        with self.assertRaises(CommError):
            self.chip.read_memory(0)


class DS2476Tests(unittest.TestCase):
    def setUp(self):
        self.ad, self.bus = make_adapter()
        self.coproc = DS2476(self.ad)

    def tearDown(self):
        self.ad.close()

    def test_detect(self):
        self.assertTrue(self.coproc.detect())
        self.assertEqual(self.coproc.rom_no, self.bus.d2476_rom)
        self.assertEqual(self.coproc.manid, b"\xAB\xCD")
        self.assertEqual(self.ad.mode, "i2c")
        self.assertEqual(self.ad.tp.baud, 115200)

    def test_memory_roundtrip(self):
        self.coproc.write_memory(5, bytes(range(32)))
        self.assertEqual(self.coproc.read_memory(5), bytes(range(32)))

    def test_rng_protection_auth(self):
        self.assertEqual(len(self.coproc.read_rng(10)), 10)
        self.assertEqual(self.coproc.read_page_protection(3), 0x03)
        self.coproc.write_buffer(b"\x01" * 32)
        self.assertEqual(self.bus.d2476_buffer, b"\x01" * 32)
        self.assertEqual(len(self.coproc.compute_read_page_auth(0, 3)), 64)   # ECDSA
        self.assertEqual(len(self.coproc.compute_read_page_auth(0, 1)), 32)   # HMAC

    def test_unknown_command_gives_device_error(self):
        with self.assertRaises(DeviceError) as cm:
            self.coproc._result_only(0x01, [0], 0, "bogus")
        self.assertEqual(cm.exception.code, 0x77)

    def test_permanent_op_gated(self):
        with self.assertRaises(PermissionError):
            self.coproc.set_page_protection(0, 0x02)


class MixedSessionTests(unittest.TestCase):
    def test_switching_between_chips(self):
        ad, bus = make_adapter()
        e38, coproc = DS28E38(ad), DS2476(ad)
        with ad:
            e38.detect()
            self.assertEqual(ad.mode, "1-wire")
            self.assertTrue(coproc.detect())
            self.assertEqual(ad.mode, "i2c")
            e38.write_memory(1, bytes([7] * 32))          # back to 1-Wire
            self.assertEqual(ad.mode, "1-wire")
            self.assertEqual(coproc.read_page_protection(0), 0x03)
            self.assertEqual(ad.mode, "i2c")
            self.assertEqual(e38.read_memory(1), bytes([7] * 32))


if __name__ == "__main__":
    unittest.main(verbosity=2)
