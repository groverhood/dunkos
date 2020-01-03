#ifndef DUNKOS_AHCI_H
#define DUNKOS_AHCI_H

#include <asm.h>

void init_ahci(void);
ssize_t ahci_write(const void *, size_t sector, size_t bytes, size_t ofs);
ssize_t ahci_read(void *, size_t sector, size_t bytes, size_t ofs);

#endif