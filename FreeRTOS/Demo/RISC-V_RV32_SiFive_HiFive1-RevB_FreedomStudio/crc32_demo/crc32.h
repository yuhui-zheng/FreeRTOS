#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>

uint32_t xcrc32 (const uint8_t *buf, uint32_t len, uint32_t init) __attribute__((section(".crc32")));

#endif /* CRC32_H */
