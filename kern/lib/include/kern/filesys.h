#ifndef DUNKOS_FILESYS_H
#define DUNKOS_FILESYS_H

#include <mode.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Maximum non-zero characters in a name. */
#define NAME_MAX 32
/* Length of a name string, null terminator included. */
#define NAME_LENGTH (NAME_MAX + 1)

typedef ssize_t off_t; 

#define FILEINFO_DIRECTORY (1 << 0)
#define FILEINFO_SYMBOLIC  (1 << 1)
#define FILEINFO_PSEUDO    (1 << 2)
#define FILEINFO_TEMPORARY (1 << 3)

void init_filesystem(void);

/* File stream object. */
struct file;

/* Subtype of file. */
struct dir;

/* Creates a blank file. Can be either a directory or a file. */
bool create_file(struct dir *relative, const char *path, mode_t);
bool remove_file(struct dir *relative, const char *path);

struct dir *get_root_dir(void);
struct file *open_file(struct dir *relative, const char *path);

void file_close(struct file *);
void file_set_info(struct file *, uint64_t infomask);

ssize_t file_read(struct file *, void *dest, size_t count);
ssize_t file_read_at(struct file *, void *dest, off_t at, size_t count);
ssize_t file_write(struct file *, const void *src, size_t count);
ssize_t file_write_at(struct file *, const void *src, off_t at, size_t count);
bool file_eof(struct file *);
size_t file_length(struct file *);
bool file_isdir(struct file *);

inline static struct dir *file_to_directory(struct file *f)
{
	return (struct dir *)f;
}

inline static struct file *dir_to_file(struct dir *d)
{
	return (struct file *)d;
}

bool create_dir(struct dir *relative, const char *path);
bool remove_dir(struct dir *relative, const char *path);
struct dir *open_dir(struct dir *relative, const char *path);
bool dir_remove(struct dir *, char name[NAME_LENGTH]);
bool dir_add_file(struct dir *, off_t sector, char name[NAME_LENGTH]);

inline static void dir_close(struct dir *dir)
{
	file_close(dir_to_file(dir));
}

#endif