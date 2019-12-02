#ifndef DUNKOS_IDE_H
#define DUNKOS_IDE_H

#include <stdint.h>
#include <stddef.h>

/* Size of a sector in bytes. */
#define SECTOR_SIZE ((size_t)512)

void init_ide(void);
void ide_read_sectors(uint32_t start, int sectors, void *dest);
void ide_write_sectors(uint32_t start, int sectors, void *src);

#endif