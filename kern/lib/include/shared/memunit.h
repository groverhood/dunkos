#ifndef DUNKOS_MEMUNIT_H
#define DUNKOS_MEMUNIT_H

#define KB (0x400)
#define MB (KB * KB)
#define GB (MB * MB)
#define PAGESIZE (2 * MB)
#define ADDRBITS (48UL)
#define ADDRMASK (~(~(1UL << (ADDRBITS - 1)) + 1))
#define PHYSSIZE (0x100000000)

#endif