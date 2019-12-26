#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <system.h>
#include <stdatomic.h>

#define CACHSZ (0x100)

struct _fdbuf {
    int fd; /* Used to access process's fdtable. */
    int error; /* Error code. */
    bool rd, wr; /* Immutable read/write flags. */
    off_t rdpos, wrpos; /* Offset into read/write buffer. */
    size_t maxrd, maxwr; /* Max offset into read/write buffer. */
    uint8_t *rdcache; /* Read buffer. */
    uint8_t *wrcache; /* Write buffer. */
};

static FILE _stdin, _stdout, _stderr;

FILE *stdin;
FILE *stdout;
FILE *stderr;

static void init_stdfile(FILE *stream, int fd, bool wr, bool rd)
{
    stream->fd = fd;
    stream->wr = wr;
    stream->rd = rd;
    if (rd) {
        stream->rdcache = calloc(CACHSZ, sizeof *stream->rdcache);
    }
    if (wr) {
        stream->wrcache = calloc(CACHSZ, sizeof *stream->wrcache);
    }
}

void init_stdio(void)
{
    stdin = &_stdin;
    stdout = &_stdout;
    stderr = &_stderr;

    init_stdfile(stdin, 0, false, true);
    init_stdfile(stdout, 1, true, false);
    init_stdfile(stderr, 2, true, false);
}

FILE *fopen(const char *path, const char *modes)
{
    FILE *stream;
    int flags;
    char modechr = *modes;
    switch (modechr) {
        case 'w': flags = OPEN_WR; break;
        case 'r': flags = OPEN_RD; break;
        case 'a': flags = OPEN_WR; break;
    }

    if (modes[1] != 0) {
        flags = OPEN_RW;
    }

    bool rd = false, wr = false;
    size_t allocsz = sizeof *stream;
    if (flags & OPEN_RD) {
        rd = true;
        allocsz += CACHSZ;
    }

    if (flags & OPEN_WR) {
        wr = true;
        allocsz += CACHSZ;
    }

    stream = calloc(1, allocsz);
    stream->rd = rd;
    stream->wr = wr;
    stream->rdpos = 0;
    stream->wrpos = 0;
    stream->maxrd = 0;
    stream->maxwr = 0;
    stream->error = 0;
    stream->rdcache = (rd) ? ((uint8_t *)stream + sizeof *stream) : NULL;
    if (wr) {
        stream->wrcache = (rd) ? (stream->rdcache + CACHSZ) : ((uint8_t *)stream + sizeof *stream);
    } else {
        stream->wrcache = NULL;
    }
    
    return stream;
}

void fclose(FILE *stream)
{
    fflush(stream);
    close(stream->fd);
    free(stream->rdcache);
    free(stream);
}

void fflush(FILE *stream)
{
    flockfile(stream);
    if (stream->wrpos > 0) {
        write(stream->fd, stream->wrcache, (size_t)stream->wrpos);
        stream->wrpos = 0;
    }
    funlockfile(stream);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t nread = 0;
    if (stream->rd) {
        size_t rdsz = size * nmemb;
        flockfile(stream);
        if (rdsz < stream->maxrd) {
            if (stream->rdpos + rdsz >= stream->maxrd) {
                stream->maxrd = (size_t)read(stream->fd, stream->rdcache, CACHSZ);
                stream->rdpos = 0;
            }
            memcpy(ptr, stream->rdcache, rdsz);
            nread += rdsz;
        } else {
            memcpy(ptr, stream->rdcache, stream->maxrd);
            nread += stream->maxrd;
            rdsz -= stream->maxrd;
            if (rdsz > 0) {
                nread += read(stream->fd, ptr, rdsz);
            }
            stream->maxrd = 0;
            stream->rdpos = 0;
        }
        funlockfile(stream);
    }

    return nread;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t nwritten = 0;
    if (stream->wr) {
        size_t wrsz = size * nmemb;
        flockfile(stream);
        if (wrsz < stream->maxwr) {
            if (stream->wrpos + wrsz >= CACHSZ) {
                fflush(stream);
            }
            memcpy(stream->wrcache, ptr, wrsz);
            stream->wrpos += wrsz;
        } else {
            fflush(stream);
            nwritten += write(stream->fd, ptr, wrsz);
        }
        funlockfile(stream);
    }

    return nwritten;
}

int feof(FILE *stream)
{
    flockfile(stream);
    int streameof = eof(stream->fd);
    funlockfile(stream);
    return streameof;
}

int ferror(FILE *stream)
{
    flockfile(stream);
    int streamerr = stream->error;
    funlockfile(stream);
    return streamerr;
}

int fileno(FILE *stream)
{
    return stream->fd;
}

static int dupformat(const char *format, va_list argv, char **dst, size_t *wrsz)
{
    int argc = 0;
    /* Copy *argv*. */
    va_list cpy;
    va_copy(cpy, argv);
    /* Look ahead to see how much we have to reserve for
       this write. */
    size_t allocsz = vsnprintf(NULL, 0, format, cpy);
    va_end(cpy);
    /* Write into a reserved buffer specifically made for
       this sized write. */
    char *bfr = calloc(allocsz, sizeof *bfr);
    va_copy(cpy, argv);
    argc += vsnprintf(bfr, allocsz, format, argv);
    va_end(cpy);
    /* Return values. */
    *dst = bfr;
    *wrsz = allocsz;
    return argc;
}

int vdprintf(int fd, const char *format, va_list argv)
{
    char *bfr;
    size_t bfrsiz;
    int argc = dupformat(format, argv, &bfr, &bfrsiz);
    write(fd, bfr, bfrsiz);
    free(bfr);
    return argc;
}

int dprintf(int fd, const char *format, ...)
{
    int argc;
    va_list argv;

    va_start(argv, format);
    argc = vdprintf(fd, format, argv);
    va_end(argv);

    return argc;
}

int vfprintf(FILE *stream, const char *format, va_list argv)
{
    int argc = 0;
    if (stream->wr) {
        char *bfr;
        size_t allocsz;
        argc += dupformat(format, argv, &bfr, &allocsz);
        fwrite(bfr, sizeof *bfr, allocsz, stream);
        free(bfr);
    }
    return argc;
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list argv;
    va_start(argv, format);

    int argc = vfprintf(stream, format, argv);

    va_end(argv);
    return argc;
}

int fputc(int c, FILE *stream)
{
    int res = c;
    unsigned char val = c;
    fwrite(&val, sizeof val, 1, stream);

    if (ferror(stream) != 0) {
        res = EOF;
    }

    return res;
}

int vfscanf(FILE *stream, const char *format, va_list argv)
{

}

int fscanf(FILE *stream, const char *format, ...)
{

}

void flockfile(FILE *stream)
{

}

void funlockfile(FILE *stream)
{

}
