#ifndef DUNKOS_FDTABLE_H
#define DUNKOS_FDTABLE_H

#include <stddef.h>
#include <stdbool.h>

/* Abstract I/O stream object. */
struct fd;
struct fdtable;

/* These are defined elsewhere. */
struct socketinfo;
struct dir;

struct fdtable *create_fdtable(struct dir *cwd, size_t);
void fdtable_destroy(struct fdtable *);

size_t fdtable_size(struct fdtable *);

struct fd *fdtable_memory(struct fdtable *);
struct fd *fdtable_socket(struct fdtable *, struct socketinfo *);
struct fd *fdtable_open(struct fdtable *, const char *path);
struct fd *fdtable_lookupfd(struct fdtable *, int fd);
void fdtable_free(struct fdtable *, int fd);

ssize_t fd_write(struct fd *, const void *, size_t);
ssize_t fd_read(struct fd *, void *, size_t);
bool fd_chmod(struct fd *, mode_t);

bool fd_eof(struct fd *);
int fd_get(struct fd *);

#endif