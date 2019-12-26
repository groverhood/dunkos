#include <filesys.h>
#include <block.h>
#include <ide.h>
#include <util/bitmap.h>
#include <util/debug.h>
#include <heap.h>
#include <util/hashtable.h>
#include <synch.h>
#include <algo.h>
#include <string.h>

static struct block *fs_block;
static struct bitmap *fs_free_map;
static struct lock fs_free_map_lock;
static struct hashtable inode_map;
static struct lock inode_map_lock;

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
	uint32_t length;
	off32_t parent;
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
		off32_t *indices = calloc(INODE_DISK_INDICES, sizeof *indices);
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

	if (idisk->next_second_level != 0) {
		bool ignore_cond = (cond == NULL);
		bool ignore_indices = (index_tablea == NULL);
		off32_t *sndlvl_indices = calloc(INODE_DISK_INDICES, sizeof *sndlvl_indices);
		block_read(fs_block, sndlvl_indices, idisk->third_level_table, BLOCK_SECTOR_SIZE);

		off32_t *fstlvl_indices = calloc(INODE_DISK_INDICES, sizeof *sndlvl_indices);
		struct inode_extent *extents = calloc(INODE_DISK_EXTENTS, sizeof *extents);

		size_t sndlvl, fstlvl, extent;
		for (sndlvl = 0; sndlvl < idisk->next_second_level - 1 && (ignore_cond || cond(aux)); ++sndlvl) {
			block_read(fs_block, fstlvl_indices, sndlvl_indices[sndlvl], BLOCK_SECTOR_SIZE);
			for (fstlvl = 0; fstlvl < INODE_DISK_INDICES && (ignore_cond || cond(aux)); ++fstlvl) {
				block_read(fs_block, extents, fstlvl_indices[fstlvl], BLOCK_SECTOR_SIZE);
				for (extent = 0; extent < INODE_DISK_EXTENTS && (ignore_cond || cond(aux)); ++extent) {
					exta(extents + extent, aux);
				}
				if (!ignore_indices && (ignore_cond || cond(aux))) {
					index_tablea(fstlvl_indices[fstlvl], aux);
				}
			}
			if (!ignore_indices && (ignore_cond || cond(aux))) {
				index_tablea(sndlvl_indices[sndlvl], aux);
			}
		}

		block_read(fs_block, fstlvl_indices, sndlvl_indices[idisk->next_second_level - 1], BLOCK_SECTOR_SIZE);
		for (fstlvl = 0; fstlvl < idisk->next_first_level - 1 && (ignore_cond || cond(aux)); ++fstlvl) {
			block_read(fs_block, extents, fstlvl_indices[fstlvl], BLOCK_SECTOR_SIZE); 
			for (extent = 0; extent < INODE_DISK_EXTENTS && (ignore_cond || cond(aux)); ++extent) {
				exta(extents + extent, aux);
			}

			if (!ignore_indices && (ignore_cond || cond(aux))) {
				index_tablea(fstlvl_indices[fstlvl], aux);
			}
		}

		block_read(fs_block, extents, fstlvl_indices[idisk->next_first_level - 1], BLOCK_SECTOR_SIZE);
		if (!ignore_indices && (ignore_cond || cond(aux))) {
			index_tablea(fstlvl_indices[idisk->next_first_level - 1], aux);
		}
		
		for (extent = 0; extent < idisk->next_extent && (ignore_cond || cond(aux)); ++extent) {
			exta(extents + extent, aux);
		}

		free(sndlvl_indices);
		free(fstlvl_indices);
		free(extents);
	}
}

static void inode_disk_foreach(struct inode_disk *idisk, inode_extent_action *exta, 
							   inode_index_table_action *index_tablea,
							   inode_foreach_conditional *cond, void *aux)
{
	assert(inode_disk_at_header(idisk) || inode_disk_at_first_level(idisk) ||
		inode_disk_at_second_level(idisk) || inode_disk_at_third_level(idisk));

	if (inode_disk_at_header(idisk)) {
		inode_disk_foreach_header(idisk, exta, cond, aux);
	} else if (inode_disk_at_first_level(idisk)) {
		inode_disk_foreach_first_level(idisk, exta, cond, aux);
	} else if (inode_disk_at_second_level(idisk)) {
		inode_disk_foreach_second_level(idisk, exta, index_tablea, cond, aux);
	} else if (inode_disk_at_third_level(idisk)) {
		inode_disk_foreach_third_level(idisk, exta, index_tablea, cond, aux);
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
	struct lock lock;
	struct lock dir_lock;
	size_t open_count;
	size_t deny_write_count;
	bool removed;
	struct inode_disk data;
};

static inline size_t hash_inode(struct list_elem *inode_elem)
{
	return hash_uint64((uint64_t)elem_value(inode_elem, struct inode, inode_map_elem)->sector);
}

static inline bool inode_equal(struct list_elem *left, struct list_elem *right)
{
	return (elem_value(left, struct inode, inode_map_elem)->sector 
			== elem_value(right, struct inode, inode_map_elem)->sector);
}

#define ROOT_DIR_SECTOR (0)

void init_filesystem(void)
{
	assert(sizeof(struct inode_disk) == BLOCK_SECTOR_SIZE);

	fs_block = get_block(BLOCK_FS);
	fs_free_map = bitmap_create(block_size(fs_block));
	hashtable_init(&inode_map, &hash_inode, &inode_equal);
	lock_init(&fs_free_map_lock);
	lock_init(&inode_map_lock);

	bitmap_set(fs_free_map, ROOT_DIR_SECTOR, 1, true);
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

static bool create_inode(size_t size, uint64_t info, off_t *sector)
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
		
			if (success && sector != NULL)
				*sector = header;
		}

		free(idisk);
	}

	return success;
}

static struct inode *get_inode(off_t sector)
{
	struct inode key = { .sector = sector };
	lock_acquire(&inode_map_lock);
	struct list_elem *inode_el = hashtable_find(&inode_map, &key.inode_map_elem);
	lock_release(&inode_map_lock);
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
		lock_init(&node->lock);
		lock_init(&node->dir_lock);
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

static void inode_free_data(struct inode_disk *node)
{
	inode_disk_foreach(node, &inode_extent_free, NULL, NULL, NULL);
}

#define offset_calc { off32_t result; off_t cur; off_t goal; }

static void calcuate_offset(struct inode_extent *ext, void *result_)
{
	struct offset_calc *result = result_;
}

static bool continue_calculating(void *result_)
{
	return (((struct offset_calc *)result_)->result == INODE_DISK_NPOS);
}

static off32_t inode_offset(struct inode *inode, off_t off)
{
	struct offset_calc c;
	inode_disk_foreach(&inode->data, &calcuate_offset, NULL, &continue_calculating, &c);
	return c.result;
}

#undef offset_calc

static ssize_t inode_read_at(struct inode *inode, void *dest, off_t at, size_t count)
{
	lock_acquire(&inode->lock);
	off32_t ofs = inode_offset(inode, at);
	lock_release(&inode->lock);
	return block_read(fs_block, dest, ofs, count); 
}

static ssize_t inode_write_at(struct inode *inode, const void *src, off_t at, size_t count)
{
	lock_acquire(&inode->lock);
	off32_t ofs = inode_offset(inode, at);
	lock_release(&inode->lock);
	return block_write(fs_block, src, ofs, count);
}

static void inode_close(struct inode *node)
{
	lock_acquire(&inode_map_lock);
	lock_acquire(&node->lock);
	if (--node->open_count == 0) {
		if (node->removed)
			inode_free_data(node);
		else
			block_write(fs_block, &node->data, node->sector, BLOCK_SECTOR_SIZE);

		hashtable_remove(&inode_map, &node->inode_map_elem);
		free(node);
	} else {
		lock_release(&node->lock);
	}
	lock_release(&inode_map_lock);
}

struct file {
	struct inode *inode;
	off_t streampos;
	bool is_directory;
};

struct dir {
	struct file base;
	off_t next_entry;
};

struct dir_entry { 
	char name[NAME_LENGTH]; // TODO: fix possible macro evaluation glitch? 
	off32_t sector; 
};

#define DIR_BASE_OFS (sizeof(struct dir_entry) * 2)

static off_t dir_get_mapping(struct dir *dir, char name[NAME_LENGTH],
							 off_t *dir_off);
static void dir_add_mapping(struct dir *dir, char name[NAME_LENGTH]);

const char *resolve_path(struct dir *root, const char *path, struct dir **parent)
{
	size_t pathstrlen = strlen(path);
}

bool create_file(struct dir *relative, const char *path, mode_t mode)
{
	struct dir *parent;
	const char *filename = resolve_path(relative, path, &parent);
	off_t header_sector;

	bool success = (create_inode(0, mode << FILEINFO_MODE_BASE, &header_sector)
					&& dir_add_file(parent, header_sector, filename));

	dir_close(parent);
	free(filename);

	return success;
}

bool remove_file(struct dir *relative, const char *path)
{
	struct dir *parent;
	const char *filename = resolve_path(relative, path, &parent);

	bool success = dir_remove(parent, filename);

	dir_close(parent);
	free(filename);

	return success;
}

static void init_file(struct file *file, struct inode *inode)
{
	file->is_directory = false;
	file->streampos = 0;
	file->inode = inode;
	lock_acquire(&inode->lock);	
	lock_release(&inode->lock);
}

struct file *open_file(struct dir *relative, const char *path)
{
	struct dir *parent;
	const char *filename = resolve_path(relative, path, &parent);
	struct file *file = NULL;

	if (filename != NULL) {
		file = calloc(1, sizeof *file);
		init_file(file, get_inode(dir_get_mapping(parent, filename, NULL)));
	}

	free(filename);
	dir_close(parent);

	return file;
}

void file_close(struct file *f)
{
	if (f != NULL) {
		inode_close(f->inode);
		free(f);
	}
}

void file_set_info(struct file *f, uint64_t infomask)
{
	struct inode *inode = f->inode;
	lock_acquire(&inode->lock);
	inode->data.info = infomask;
	lock_release(&inode->lock);
}

ssize_t file_read(struct file *f, void *dest, size_t count)
{
	ssize_t bytes_read = file_read_at(f, dest, f->streampos, count);
	f->streampos += bytes_read;
	return bytes_read;
}

ssize_t file_read_at(struct file *f, void *dest, off_t at, size_t count)
{
	return inode_read_at(f->inode, dest, at, count);
}

ssize_t file_write(struct file *f, const void *src, size_t count)
{
	ssize_t bytes_written = file_write_at(f, src, f->streampos, count);
	f->streampos += bytes_written;
	return bytes_written;
}

ssize_t file_write_at(struct file *f, const void *src, off_t at, size_t count)
{
	return inode_write_at(f->inode, src, at, count);
}

bool file_eof(struct file *f)
{
	struct inode *inode = f->inode;
	lock_acquire(&inode->lock);
	bool eof = (f->streampos == inode->data.length);
	lock_release(&inode->lock);
	return eof;
}

bool file_isdir(struct file *f)
{
	return f->is_directory;
}

static bool init_dir(struct dir *relative, const char *path)
{
	struct dir *dir = open_dir(relative, path);
	bool success;
	if (relative != NULL) {
		success = dir_add_file(dir, dir_to_file(dir)->inode->data.parent, "..");
	} else {
		success = dir_add_file(dir, dir_to_file(dir)->inode->sector, "..");
	}

	success = success && dir_add_file(dir, dir_to_file(dir)->inode->sector, ".");
	file_close(dir);
	return success;
}

bool create_dir(struct dir *relative, const char *path)
{
	return create_file(relative, path, 0666) && init_dir(relative, path);
}

struct dir *open_dir(struct dir *relative, const char *path)
{
	struct dir *parent, *dir = NULL;
	const char *filename = resolve_path(relative, path, &parent);
	struct inode *inode = NULL;

	if (filename != NULL) {
		inode = get_inode(dir_get_mapping(parent, filename, NULL));
	} else if (parent != NULL) {
		inode = get_inode(dir_to_file(relative)->inode->sector);
	}

	if (inode != NULL) {
		dir = calloc(1, sizeof *dir);
		init_file(dir_to_file(dir), inode);
		dir_to_file(dir)->is_directory = true;
		dir->next_entry = DIR_BASE_OFS;
	}

	dir_close(parent);
	free(filename);

	return dir;
}

static off_t dir_get_mapping(struct dir *dir, char name[NAME_LENGTH], off_t *entryofs)
{
	struct dir_entry ent;
	off_t result = INODE_DISK_NPOS;
	off_t ofs;

	for (ofs = 0; ofs < BLOCK_SECTOR_SIZE; ofs += sizeof ent) {
		file_read_at(dir_to_file(dir), &ent, ofs, sizeof ent);

		if (!strncmp(ent.name, name, NAME_MAX))	{
			if (entryofs != NULL) {
				*entryofs = ofs;
			}
			result = ent.sector;
			break;
		}	
	}

	return result;
}

bool dir_remove(struct dir *dir, char name[NAME_LENGTH])
{
	off_t entryofs;
	off_t inode_sector = dir_get_mapping(dir, name, &entryofs);
	bool success = false;

	if (inode_sector != INODE_DISK_NPOS) {
		static char ZEROES[sizeof(struct dir_entry)];
		success = true;
		file_write_at(dir, ZEROES, entryofs, sizeof(struct dir_entry));

		struct inode_disk *data = calloc(1, sizeof *data);
		block_read(fs_block, data, inode_sector, sizeof *data);
		inode_free_data(data);
		free(data);
	}
}

bool dir_add_file(struct dir *dir, off_t sector, char name[NAME_LENGTH])
{
	off_t emptyofs;
	bool success = false;
	if (dir_get_mapping(dir, name, &emptyofs) != INODE_DISK_NPOS) {
		struct dir_entry new_ent = { .sector = sector, .name = name };
		success = (file_write_at(dir, &new_ent, emptyofs, sizeof new_ent) == sizeof new_ent);
	}
	return success;
}

struct dir *get_root_dir(void)
{
	struct dir *root = calloc(1, sizeof *root);
	struct file *file = dir_to_file(root);

	file->inode = get_inode(ROOT_DIR_SECTOR);
	file->is_directory = true;
	file->streampos = 0;

	root->next_entry = DIR_BASE_OFS;
	return root;
}
