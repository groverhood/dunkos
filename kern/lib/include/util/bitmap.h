#ifndef DUNKOS_UTIL_BITMAP_H
#define DUNKOS_UTIL_BITMAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define BITMAP_NPOS ((size_t)-1)

typedef bool bit;

struct bitmap;

struct bitmap *bitmap_create(size_t bits);
struct bitmap *bitmap_create_buffer(size_t bits, void *, size_t size);

size_t bitmap_size(struct bitmap *);

static inline size_t bitmap_bufsize(size_t bits)
{
	return (bits / 8 + !!(bits & 07));
}

void bitmap_set(struct bitmap *, size_t start, size_t nbits, bit);
void bitmap_flip(struct bitmap *bm, size_t start, size_t nbits);
size_t bitmap_scan(struct bitmap *, size_t start, size_t nbits, bit);
size_t bitmap_scan_and_flip(struct bitmap *, size_t start, size_t nbits, bit);

#endif