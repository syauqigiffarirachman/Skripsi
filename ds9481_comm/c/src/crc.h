/* Dallas/Maxim CRCs. */
#ifndef CRC_H
#define CRC_H

#include <stddef.h>
#include <stdint.h>

#define CRC16_RESIDUE 0xB001u  /* crc16 over (data + inverted CRC16) always ends here */

uint8_t  crc8(const uint8_t *d, size_t n, uint8_t crc);     /* result 0 over data+CRC8 */
uint16_t crc16(const uint8_t *d, size_t n, uint16_t crc);

#endif
