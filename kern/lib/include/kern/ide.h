#ifndef DUNKOS_IDE_H
#define DUNKOS_IDE_H

#include <stdint.h>
#include <stddef.h>

#define SECTORS_PER_BUNCH ((size_t)256)

void init_ide(void);
void ide_read_sectors(uint32_t start, int sectors, void *dest);
void ide_write_sectors(uint32_t start, int sectors, void *src);

#endif