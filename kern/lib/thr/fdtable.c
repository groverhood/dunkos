#include <kern/fdtable.h>
#include <kern/heap.h>
#include <kern/filesys.h>
#include <kern/synch.h>
#include <util/hashtable.h>
#include <util/bitmap.h>
#include <algo.h>
#include <string.h>

enum fd_type {
    FDTYPE_FILE, /* File stream. */
    FDTYPE_MEM, /* In-memory stream. */
    FDTYPE_NET /* Network stream. */
};

/* Generic I/O function prototype. */
typedef ssize_t fd_read_func(void *object, void *, size_t);
typedef ssize_t fd_write_func(void *object, const void *, size_t);

struct fd {
    int fd;
    enum fd_type type;
    struct list_elem bucket_elem;
    void *io_object;
    fd_read_func *readfunc;
    fd_write_func *writefunc;
    struct lock lock;
};

struct fdtable {
    struct hashtable fdmap;
    struct bitmap *freefdmap;
    struct dir *cwd;
    struct lock lock;
};

static size_t hash_fd(struct list_elem *e)
{
    return hash_uint64(elem_value(e, struct fd, bucket_elem)->fd);
}

static bool fd_identity(struct list_elem *l, struct list_elem *r)
{
    struct fd *leftfd = elem_value(l, struct fd, bucket_elem);
    struct fd *rightfd = elem_value(r, struct fd, bucket_elem);

    return (leftfd->fd == rightfd->fd);
}

struct fdtable *create_fdtable(struct dir *cwd, size_t cnt)
{
    struct fdtable *fdtable = calloc(1, sizeof *fdtable);

    hashtable_init(&fdtable->fdmap, &hash_fd, &fd_identity);
    fdtable->freefdmap = bitmap_create(cnt);
    fdtable->cwd = cwd;
    lock_init(&fdtable->lock);

    return fdtable;
}

static void fd_free(struct fd *fd)
{
    switch (fd->type) {
        case FDTYPE_FILE: file_close(fd->io_object); break;
    }

    free(fd);
}

static void fd_destroy(struct list_elem *e, void *aux)
{
    fd_free(elem_value(e, struct fd, bucket_elem));
}

void fdtable_destroy(struct fdtable *fdtable)
{
    hashtable_destroy(&fdtable->fdmap, &fd_destroy, NULL);
}

size_t fdtable_size(struct fdtable *fdtable)
{
    return bitmap_size(fdtable->freefdmap);
}

struct fd *fdtable_open(struct fdtable *fdtable, const char *path)
{
    struct fd *fd = calloc(1, sizeof *fd);
    lock_acquire(&fdtable->lock);
    hashtable_insert(&fdtable->fdmap, &fd->bucket_elem);

    struct file *file = open_file(fdtable->cwd, path);
    fd->io_object = file;

    lock_init(&fd->lock);
    fd->readfunc = (fd_read_func *)&file_read;
    fd->writefunc = (fd_write_func *)&file_write;
    fd->fd = bitmap_scan_and_flip(fdtable->freefdmap, 0, 1, true);
    lock_release(&fdtable->lock);

    return fd;
}

struct fd *fdtable_lookupfd(struct fdtable *fdtable, int fd)
{
    struct fd key = { .fd = fd }, *foundfd = NULL;
    struct list_elem *el = hashtable_find(&fdtable->fdmap, &key.bucket_elem);
    if (el != NULL) {
        foundfd = elem_value(el, struct fd, bucket_elem);
    }
    return foundfd;
}

void fdtable_free(struct fdtable *fdtable, int fd)
{
    fd_free(fdtable_lookupfd(fdtable, fd));
}

ssize_t fd_write(struct fd *fd, const void *src, size_t bytes)
{
    lock_acquire(&fd->lock);
    ssize_t bytes_written = fd->writefunc(fd->io_object, src, bytes);
    lock_release(&fd->lock);
    return bytes_written;
}

ssize_t fd_read(struct fd *fd, void *dest, size_t bytes)
{
    lock_acquire(&fd->lock);
    ssize_t bytes_read = fd->readfunc(fd->io_object, dest, bytes);
    lock_release(&fd->lock);
    return bytes_read;
}

bool fd_eof(struct fd *fd)
{
    /* Default to false so as to prevent I/O on an invalid file descriptor. */
    bool eof = false;
    lock_acquire(&fd->lock);
    switch (fd->type) {
        case FDTYPE_FILE: eof = file_eof(fd->io_object);
    }
    lock_release(&fd->lock);
    return eof;
}

int fd_get(struct fd *fd)
{
    /* Synchronization doesn't matter here, this data is immutable. */
    return fd->fd;
}