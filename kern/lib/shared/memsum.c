#include <stdext.h>

size_t memsum(void *ptr, size_t n) 
{
    unsigned char *bptr = ptr;
    size_t sum = 0;
    while (n-- > 0) {
        sum += *bptr++;
    }

    return sum;
}