#include <kern/filesys.h>
#include <kern/block.h>
#include <kern/ide.h>
#include <util/bitmap.h>
#include <util/debug.h>
#include <kern/heap.h>
#include <util/hashtable.h>
#include <kern/synch.h>
#include <algo.h>

#define INODE_HEADER_EXTENTS (59)

typedef uint32_t off32_t;

#define INODE_DISK_NPOS ((off32_t)-1)

struct inode_extent {
	off32_t start;
	uint32_t sectors;
};

#define INODE_DISK_EXTENTS (BLOCK_SECTOR_SIZE / sizeof(struct inode_extent))
#define INODE_DISK_INDICES (BLOCK_SECTOR_SIZE / sizeof(off32_t))

struct inode_disk {
	uint64_t info;
	off32_t next_extent;
	off32_t next_first_level;
	off32_t next_second_level;

	off32_t first_level_table;
	off32_t second_level_table;
	off32_t third_level_table;

	struct inode_extent header_extents[INODE_HEADER_EXTENTS];
};

typedef void inode_extent_action(struct inode_extent *, void *aux);
typedef void inode_index_table_action(off32_t, void *aux);
typedef bool inode_foreach_conditional(void *aux);

static bool inode_disk_at_header(struct inode_disk *idisk)
{
	return (idisk->first_level_table == INODE_DISK_NPOS);
}

static bool inode_disk_at_first_level(struct inode_disk *idisk)
{
	return (idisk->first_level_table != INODE_DISK_NPOS)
		&& (idisk->second_level_table == INODE_DISK_NPOS)
		&& (idisk->third_level_table == INODE_DISK_NPOS);
}

static bool inode_disk_at_second_level(struct inode_disk *idisk)
{
	return (idisk->second_level_table != INODE_DISK_NPOS)
		&& (idisk->third_level_table == INODE_DISK_NPOS);
}

static bool inode_disk_at_third_level(struct inode_disk *idisk)
{
	return (idisk->third_level_table != INODE_DISK_NPOS);
}

static void inode_disk_foreach_header(struct inode_disk *idisk, inode_extent_action *exta, 
										inode_foreach_conditional *cond, void *aux)
{
	bool ignore_cond = (cond == NULL);
	size_t num_extents = !inode_disk_at_header(idisk) ? INODE_HEADER_EXTENTS : idisk->next_extent;
	size_t i;
	for (i = 0; i < num_extents && (ignore_cond || cond(aux)); ++i)
		exta(idisk->header_extents + i, aux);
}

static void inode_disk_foreach_first_level(struct inode_disk *idisk, 
										inode_extent_action *exta, 
										inode_foreach_conditional *cond, void *aux)
{
	inode_disk_foreach_header(idisk, exta, cond, aux);

	struct inode_extent *extents = calloc(INODE_DISK_EXTENTS, sizeof *extents);
	block_read(fs_block, extents, idisk->first_level_table, BLOCK_SECTOR_SIZE);

	bool ignore_cond = (cond == NULL);
	size_t num_extents = !inode_disk_at_first_level(idisk) ? INODE_DISK_EXTENTS : idisk->next_extent;
	size_t i;
	for (i = 0; i < num_extents && (ignore_cond || cond(aux)); ++i)
		exta(extents + i, aux);

	block_write(fs_block, extents, idisk->first_level_table, BLOCK_SECTOR_SIZE);
	free(extents);
}

static void inode_disk_foreach_second_level(struct inode_disk *idisk,
										inode_extent_action *exta, 
										inode_index_table_action *index_tablea,
										inode_foreach_conditional *cond, void *aux)
{
	inode_disk_foreach_first_level(idisk, exta, cond, aux);
	bool at_second_level = !inode_disk_at_second_level(idisk);

	if (!at_second_level || idisk->next_first_level != 0) {
		bool ignore_cond = (cond == NULL);
		bool ignore_indices = (index_tablea == NULL);
		struct off32_t *indices = calloc(INODE_DISK_INDICES, sizeof *indices);
		block_read(fs_block, indices, idisk->second_level_table, BLOCK_SECTOR_SIZE);

		struct inode_extent *extents = calloc(INODE_DISK_EXTENTS, sizeof *extents);
		size_t extent_blocks = !at_second_level ? INODE_DISK_INDICES : (idisk->next_first_level - 1);
		size_t i, j;
		for (i = 0; i < extent_blocks && (ignore_cond || cond(aux)); ++i) {
			block_read(fs_block, extents, indices[i], BLOCK_SECTOR_SIZE);
			if (!ignore_indices)
				index_tablea(indices[i], aux);
			for (j = 0; j < INODE_DISK_EXTENTS && (ignore_cond || cond(aux)); ++j)
				exta(extents + j, aux);
		}

		if (at_second_level) {
			block_read(fs_block, extents, idisk->next_first_level - 1, BLOCK_SECTOR_SIZE);

			for (i = 0; i < idisk->next_extent && (ignore_cond || cond(aux)); ++i)
				exta(extents + i, aux);
		}

		free(indices);
		free(extents);	
	}
}

static void inode_disk_foreach_third_level(struct inode_disk *idisk,
										inode_extent_action *exta, 
										inode_index_table_action *index_tablea,
										inode_foreach_conditional *cond, void *aux)
{
	inode_disk_foreach_second_level(idisk, exta, index_tablea, cond, aux);

}

static void inode_disk_foreach(struct inode_disk *idisk, inode_extent_action *exta, 
							   inode_index_table_action *index_tablea,
							   inode_foreach_conditional *cond, void *aux)
{
	assert(inode_disk_at_header(idisk) || inode_disk_at_first_level(idisk) ||
		inode_disk_at_second_level(idisk) || inode_disk_at_third_level(idisk));

	if (inode_disk_at_header(idisk)) {
		inode_disk_foreach_header(idisk, exta, cond);
	} else if (inode_disk_at_first_level(idisk)) {
		inode_disk_foreach_first_level(idisk, exta, cond);
	} else if (inode_disk_at_second_level(idisk)) {
		inode_disk_foreach_second_level(idisk, exta, index_tablea, cond);
	} else if (inode_disk_at_third_level(idisk)) {
		inode_disk_foreach_third_level(idisk, exta, index_tablea, cond);
	}
}

static bool inode_disk_allocate_space(struct inode_disk *idisk, size_t sectors)
{
	if (inode_disk_at_header(idisk)) {
		
	}
}

struct inode {
	off_t sector;
	struct list_elem inode_map_elem;
	size_t open_count;
	size_t deny_write_count;
	bool removed;
	struct inode_disk data;
};

static struct block *fs_block;
static struct bitmap *fs_free_map;
static struct lock fs_free_map_lock;
static struct hashtable inode_map;
static struct lock inode_map_lock;

static inline size_t hash_inode(struct list_elem *inode_elem)
{
	return hash_uint64((uint64_t)elem_value(inode_elem, struct inode, inode_map_elem)->sector);
}

static inline bool inode_equal(struct list_elem *left, struct list_elem *right)
{
	return (elem_value(left, struct inode, inode_map_elem)->sector 
			== elem_value(right, struct inode, inode_map_elem)->sector);
}

void init_filesystem(void)
{
	assert(sizeof(struct inode_disk) == BLOCK_SECTOR_SIZE);

	fs_block = get_block(BLOCK_FS);
	fs_free_map = bitmap_create(block_size(fs_block));
	hashtable_init(&inode_map, &hash_inode, &inode_equal);
	lock_init(&fs_free_map_lock);
	lock_init(&inode_map_lock);
}

static size_t free_map_allocate(size_t sectors)
{
	size_t sector;
	lock_acquire(&fs_free_map_lock);
	assert(sectors < bitmap_size(&fs_free_map));
	sector = bitmap_scan_and_flip(&fs_free_map, 0, sectors, false);
	lock_release(&fs_free_map_lock);
	return sector;
}

static void free_map_release(size_t where, size_t sectors)
{
	lock_acquire(&fs_free_map_lock);
	assert(bitmap_all(&fs_free_map, where, sectors, true));
	bitmap_set(&fs_free_map, where, sectors, false);
	lock_release(&fs_free_map_lock);
}

static bool create_inode(size_t size, uint64_t info)
{
	size_t sectors = div_rnd_up(size, BLOCK_SECTOR_SIZE);
	size_t header = free_map_allocate(1);
	bool success = (header != BITMAP_NPOS);

	if (success) {
		struct inode_disk *idisk = calloc(1, sizeof *idisk);
		success = (idisk != NULL);

		if (success) {
			idisk->info = info;

			idisk->next_extent = 0;
			idisk->next_first_level = INODE_DISK_NPOS;
			idisk->next_second_level = INODE_DISK_NPOS;

			idisk->first_level_table = INODE_DISK_NPOS;
			idisk->second_level_table = INODE_DISK_NPOS;
			idisk->third_level_table = INODE_DISK_NPOS;

			success = (sectors == 0 || inode_disk_allocate_space(idisk, sectors))
					&& (block_write(fs_block, idisk, header, sizeof *idisk) == sizeof *idisk);
		}

		free(idisk);
	}

	return success;
}

static struct inode *get_inode(off_t sector)
{
	struct inode key = { .sector = sector };
	struct list_elem *inode_el = hashtable_find(&inode_map, &key.inode_map_elem);
	return elem_value(inode_el, struct inode, inode_map_elem);
}

static struct inode *open_inode(off_t sector)
{
	lock_acquire(&fs_free_map_lock);
	assert(sector < bitmap_size(&fs_free_map));
	assert(bitmap_all(fs_free_map, sector, 1, 1));
	lock_release(&fs_free_map_lock);

	lock_acquire(&inode_map_lock);
	struct inode *node = get_inode(sector);

	if (node == NULL) {
		node = calloc(1, sizeof *node);
		ide_read_sectors((off32_t)sector, 1, &node->data);

		node->open_count = 1;
		node->sector = sector;
		hashtable_insert(&inode_map, &node->inode_map_elem);
	} else node->open_count++;
	lock_release(&inode_map_lock);

	return node;
}

static void inode_extent_free(struct inode_extent *ext, void *aux)
{
	lock_acquire(&fs_free_map_lock);
	assert(bitmap_all(&fs_free_map, ext->start, ext->sectors, true));
	lock_release(&fs_free_map_lock);
	free_map_release(ext->start, ext->sectors);
}

static void inode_free_data(struct inode *node)
{
	inode_disk_foreach(&node->data, &inode_extent_free, NULL, NULL, NULL);
}

static ssize_t inode_read_at(struct inode *inode, void *dest, off_t at, size_t count)
{
	return block_read(fs_block, src, , count); 
}

static ssize_t inode_write_at(struct inode *inode, const void *src, off_t at, size_t count)
{
	return block_write(fs_block, src, , count);
}

static void inode_close(struct inode *node)
{
	if (--node->open_count == 0) {
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

const char *resolve_path(struct dir *root, const char *path, struct dir **parent)
{

}

bool create_file(struct dir *relative, const char *path)
{
	struct dir *parent;
	const char *filename = resolve_path(relative, path, &parent);

	free(filename);
	return create_inode(0);
}

bool remove_file(struct dir *relative, const char *path)
{

}

struct file *open_file(struct dir *relative, const char *path)
{

}

void file_close(struct file *f)
{
	inode_close(f->inode);
	free(f);
}

void file_set_info(struct file *f, uint64_t infomask)
{
	f->inode->data.info = infomask;
}

ssize_t file_read_at(struct file *f, void *dest, off_t at, size_t count)
{
	return inode_read_at(f->inode, dest, at, count);
}

ssize_t file_write_at(struct file *f, const void *src, off_t at, size_t count)
{

}

bool file_isdir(struct file *f)
{

}

