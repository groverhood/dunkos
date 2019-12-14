#ifndef DUNKOS_BLOCK_H
#define DUNKOS_BLOCK_H

#include <stddef.h>

#define BLOCK_SECTOR_SIZE ((size_t)512)

struct block;

void init_block_devices(void);

enum block_type {
	BLOCK_KERNEL,
	BLOCK_FS,
	BLOCK_SWAP,

	BLOCK_COUNT
};

struct block *get_block(enum block_type role);

const char *block_get_name(const struct block *);

/* Get the size of a block partition in sectors. */
size_t block_size(const struct block *);

ssize_t block_read(struct block *, void *dest, size_t start, size_t bytes);
ssize_t block_write(struct block *, const void *dest, size_t start, size_t bytes);

#endif