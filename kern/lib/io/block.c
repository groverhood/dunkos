#include <block.h>
#include <synch.h>
#include <string.h>
#include <heap.h>
#include <util/debug.h>
#include <stdint.h>

struct block {
	const char *name;	
	uint32_t start;
	uint32_t size;
	struct lock lock;
	uint8_t remainder[BLOCK_SECTOR_SIZE];
};

struct block block_devices[BLOCK_COUNT];

#define DUNKOS_IMGHDR __attribute__((packed, aligned (BLOCK_SECTOR_SIZE)))
#define DUNKOS_MAGIK (*(uint64_t *)"DunkOS!")

struct DUNKOS_IMGHDR dunkos_image_header {
    uint64_t image_size;
    uint64_t fs_block_start;
    uint64_t fs_block_size;
    uint64_t swap_block_start;
    uint64_t swap_block_size;
    uint64_t magic;
};

void init_block_devices(void)
{
	size_t i;
	for (i = 0; i < BLOCK_COUNT; ++i) {
		struct block *b = (block_devices + i);
		lock_init(&b->lock);
		memset(b->remainder, 0, BLOCK_SECTOR_SIZE);
	}
	
	struct dunkos_image_header *hdr = kcalloc(1, sizeof *hdr);
	ide_read_sectors(0, 1, hdr);

	assert(hdr->magic == DUNKOS_MAGIK);

	block_devices[BLOCK_FS].start = hdr->fs_block_start;
	block_devices[BLOCK_FS].size = hdr->fs_block_size;
	block_devices[BLOCK_SWAP].start = hdr->swap_block_start;
	block_devices[BLOCK_SWAP].size = hdr->swap_block_size;

	kfree(hdr);
}

struct block *get_block(enum block_type role)
{
	return (block_devices + role);
}

const char *block_get_name(const struct block *b)
{
	return b->name;
}

size_t block_size(const struct block *b)
{
	return (size_t)b->size;
}

ssize_t block_read(struct block *b, void *dest, size_t start, size_t bytes)
{
	lock_acquire(&b->lock);
	uint32_t sector_start = b->start + (uint32_t)start;
	uint32_t sectors = (bytes / BLOCK_SECTOR_SIZE);
	uint32_t rem = (bytes % BLOCK_SECTOR_SIZE);

	ide_read_sectors(sector_start, sectors, dest);

	if (rem != 0) {
		ide_read_sectors(sector_start + sectors, 1, b->remainder);
		memcpy((uint8_t *)dest + sectors * BLOCK_SECTOR_SIZE, b->remainder, rem);
	}

	lock_release(&b->lock);
	return (ssize_t)BLOCK_SECTOR_SIZE - (ssize_t)rem;
}

ssize_t block_write(struct block *b, const void *src, size_t start, size_t bytes)
{
	lock_acquire(&b->lock);
	uint32_t sector_start = b->start + (uint32_t)start;
	uint32_t sectors = (bytes / BLOCK_SECTOR_SIZE);
	uint32_t rem = (bytes % BLOCK_SECTOR_SIZE);

	ide_write_sectors(sector_start, sectors, (void *)src);

	if (rem != 0) {
		memcpy(b->remainder, (uint8_t *)src + sectors * BLOCK_SECTOR_SIZE, rem);
		ide_write_sectors(sector_start + sectors, 1, b->remainder);
	}

	lock_release(&b->lock);
	return (ssize_t)BLOCK_SECTOR_SIZE - (ssize_t)rem;
}
