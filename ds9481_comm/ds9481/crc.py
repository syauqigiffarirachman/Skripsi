"""Dallas/Maxim CRCs used on the 1-Wire / I2C links."""

CRC16_RESIDUE = 0xB001  # CRC16 over (data + inverted CRC16) always ends here


def crc8(data, crc=0):
    """Dallas CRC8 (x^8+x^5+x^4+1, reflected 0x8C). Result 0 over data+CRC."""
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8C if crc & 1 else crc >> 1
    return crc


def crc16(data, crc=0):
    """Dallas CRC16 (x^16+x^15+x^2+1, reflected 0xA001).

    The DS28E38 sends the *inverted* CRC16 (low byte first), so a frame that
    includes those two bytes must give CRC16_RESIDUE (0xB001).
    """
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc
