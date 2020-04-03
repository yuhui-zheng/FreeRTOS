
#ifndef CRC32_H
#define CRC32_H

unsigned int xcrc32 (const unsigned char *buf, int len, unsigned int init) __attribute__((section(".crc32")));

#endif /* CRC32_H */
