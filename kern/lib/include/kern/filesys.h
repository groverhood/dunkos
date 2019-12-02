#ifndef DUNKOS_FILESYS_H
#define DUNKOS_FILESYS_H

#include <stddef.h>
#include <stdbool.h>

typedef ssize_t off_t; 

#define FILEINFO_DIRECTORY (1 << 0)
#define FILEINFO_USERMUTBL (1 << 1)
#define FILEINFO_SYMBOLIC  (1 << 2)
#define FILEINFO_PSEUDO    (1 << 3)
#define FILEINFO_TEMPORARY (1 << 4)

void init_filesystem(void);

struct file;

/* Subtype of file. */
struct dir;

/* Creates a blank file. Can be either a directory or a file. */
bool create_file(struct dir *relative, const char *path);
bool remove_file(struct dir *relative, const char *path);

struct file *open_file(struct dir *relative, const char *path);

void file_close(struct file *);
void file_set_info(struct file *, uint64_t infomask);

ssize_t file_read_at(struct file *, void *dest, off_t at, size_t count);
ssize_t file_write_at(struct file *, const void *src, off_t at, size_t count);

bool file_isdir(struct file *);

inline static struct dir *file_to_directory(struct file *f)
{
	return (struct dir *)f;
}

struct file **dir_get_files(struct dir *);
bool dir_add_file(struct dir *, struct file *);

#endif