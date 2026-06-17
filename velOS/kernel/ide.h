#ifndef IDE_H
#define IDE_H

#include <stdint.h>

void ide_init(void);
int ide_read_sectors(uint32_t lba, uint8_t count, void* buffer);
int ide_write_sectors(uint32_t lba, uint8_t count, const void* buffer);

#endif
