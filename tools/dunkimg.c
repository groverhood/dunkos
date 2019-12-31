
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define DUNKOS_BLOCK_SIZE 512
#define DUNKOS_IMGHDR __attribute__((packed, aligned (DUNKOS_BLOCK_SIZE)))
#define DUNKOS_FSTSCT 2 
#define DUNKOS_MAGIK (*(uint64_t *)"DunkOS!")

struct DUNKOS_IMGHDR dunkos_image_header {
    uint64_t image_size;
    uint64_t fs_block_start;
    uint64_t fs_block_size;
    uint64_t swap_block_start;
    uint64_t swap_block_size;
    uint64_t magic;
};

struct dunkos_partspec {
    int divs; /* Number of chunks to divide the disk into. */
    int fs, swap;
};

void fmtdunkimg(const char *image, struct dunkos_partspec *spec)
{
    assert(spec->fs + spec->swap == spec->divs);
    struct stat blnkimg_stat;
    struct dunkos_image_header *h;

    if (!stat(image, &blnkimg_stat)) {
        assert(blnkimg_stat.st_blksize == DUNKOS_BLOCK_SIZE);
        uint64_t blocks = blnkimg_stat.st_blocks;
        uint64_t chunksz = (blocks / spec->divs);
        uint64_t sector = DUNKOS_FSTSCT;

        h = calloc(1, DUNKOS_BLOCK_SIZE);
        h->image_size = (uint64_t)blocks;
        h->fs_block_start = sector;
        h->fs_block_size = (chunksz * spec->fs) + (blocks % spec->divs);
        sector += h->fs_block_size;
        h->swap_block_start = sector;
        h->swap_block_size = (chunksz * spec->swap);
        h->magic = DUNKOS_MAGIK;
        
        int imgfd = open(image, O_WRONLY);
        write(imgfd, h, DUNKOS_BLOCK_SIZE);
        close(imgfd);
    }
}
