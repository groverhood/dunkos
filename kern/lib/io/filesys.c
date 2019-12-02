#include <kern/filesys.h>
#include <kern/block.h>
#include <kern/ide.h>
#include <util/bitmap.h>
#include <util/debug.h>
#include <kern/heap.h>

#define INODE_HEADER_EXTENTS (62)

typedef uint32_t off32_t;

struct inode_extent {
	off32_t start;
	uint32_t sectors;
};

struct inode_disk {
	uint32_t info;
	uint32_t next_first_level;
	uint32_t next_second_level;
	uint32_t next_third_level;

	struct inode_extent header_extents[INODE_HEADER_EXTENTS];
};

struct inode {
	off_t sector;
	size_t open_count;
	size_t deny_write_count;
	bool removed;
	struct inode_disk data;
};

static struct block *fs_block;
static struct bitmap *fs_free_map;

void init_filesystem(void)
{
	fs_block = get_block(BLOCK_FS);
	fs_free_map = bitmap_create(block_size(fs_block));
}

static struct inode *open_inode(off_t sector)
{
	assert(bitmap_all(fs_free_map, sector, 1, 1));

	struct inode *node = calloc(1, *node);
	ide_read_sectors((off32_t)sector, 1, &node->data);

	node->open_count = 1;
	node->sector = sector;
	return node;
}

static void inode_free_data(struct inode *node)
{

}

static void inode_close(struct inode *node)
{
	node->open_count--;
	if (node->open_count == 0) {
		if (node->removed)
			inode_free_data(node);
		else
			block_write(fs_block, &node->data, node->sector, SECTOR_SIZE);

		free(node);
	}
}

struct file {
	struct inode *inode;
	char *name;
	size_t length;
	bool is_directory;
};

struct dir {
	struct file base;
	off_t next_entry;
};

bool create_file(struct dir *relative, const char *path)
{
	
}

bool remove_file(struct dir *relative, const char *path)
{

}

struct file *open_file(struct dir *relative, const char *path)
{

}

void file_close(struct file *)
{

}

void file_set_info(struct file *, uint64_t infomask)
{

}

ssize_t file_read_at(struct file *, void *dest, off_t at, size_t count)
{

}

ssize_t file_write_at(struct file *, const void *src, off_t at, size_t count)
{

}

bool file_isdir(struct file *)
{

}

