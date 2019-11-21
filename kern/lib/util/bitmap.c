#include <util/bitmap.h>
#include <kern/heap.h>
#include <string.h>

#define ELEM_BITS (sizeof(size_t) * 8)

static inline size_t bit_to_elem(size_t bit_index)
{
	return bit_index / ELEM_BITS;
}

struct bitmap {
	size_t bitcount;
	size_t bufsize;
};

static inline size_t *bitmap_elem(struct bitmap *b, size_t bit_index) 
{
	return (size_t *)(b + 1) + bit_to_elem(bit_index);
}

static inline size_t bitmap_mask(size_t bit_index)
{
	return (size_t)1 << (bit_index % ELEM_BITS);
}

struct bitmap *bitmap_create(size_t bits) 
{
	size_t bufsize = bitmap_bufsize(bits);
	struct bitmap *b = malloc(sizeof *b + bufsize);
	memset(b, 0, sizeof *b + bufsize);

	b->bitcount = bits;
	b->bufsize = bufsize;

	return b;
}

struct bitmap *bitmap_create_buffer(size_t bits, void *buf, size_t size) 
{
	struct bitmap *b = buf;

	b->bitcount = bits;
	b->bufsize = size;

	return b;
}

size_t bitmap_size(struct bitmap *bmap) 
{
	return bmap->bitcount;
}

static inline bool bitmap_test(struct bitmap *bm, size_t bit_index)
{
	return (*bitmap_elem(bm, bit_index) & bitmap_mask(bit_index)) != 0;
}

void bitmap_set(struct bitmap *bm, size_t start, size_t nbits, bit b) 
{
	while (start < bm->bitcount && nbits-- > 0) {
		size_t *elem_ptr = bitmap_elem(bm, start);
		size_t elem_mask = bitmap_mask(start++);
		if (b)
			__asm__ ("orq %1, %0" : "=m" (*elem_ptr) : "r" (elem_mask) : "cc");
		else
			__asm__ ("andq %1, %0" : "=m" (*elem_ptr) : "r" (~elem_mask) : "cc");
	}
}

void bitmap_flip(struct bitmap *bm, size_t start, size_t nbits)
{
	while (start < bm->bitcount && nbits-- > 0) {
		size_t *elem_ptr = bitmap_elem(bm, start);
		size_t elem_mask = bitmap_mask(start++);
		__asm__ ("xorq %1, %0" : "=m" (*elem_ptr) : "r" (elem_mask) : "cc");
	}
}

size_t bitmap_scan(struct bitmap *bm, size_t start, size_t nbits, bit b) 
{
	bool bit_found = false;
	while (start < bm->bitcount && !bit_found) {
		size_t bits_scanned = 0;
		while (bits_scanned < nbits 
				&& bitmap_test(bm, start + bits_scanned) == b) {
			bits_scanned++;	
		}
		bit_found = (bits_scanned == nbits);

		if (!bit_found) {
			start += bits_scanned;
		}
	}

	return bit_found ? start : BITMAP_NPOS;
}

size_t bitmap_scan_and_flip(struct bitmap *bm, size_t start, size_t nbits, bit b) 
{
	size_t index = bitmap_scan(bm, start, nbits, b);
	if (index != BITMAP_NPOS)	
		bitmap_flip(bm, index, nbits);
	return index;
}