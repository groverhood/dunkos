#ifndef DUNKOS_MODE_H
#define DUNKOS_MODE_H

#define FILEINFO_MODE_BASE (8)

#define FILEINFO_MODE_X	(0b001)
#define FILEINFO_MODE_W (0b010)
#define FILEINFO_MODE_R (0b100)

#define FILEINFO_MODE_SIZE (3)
#define FILEINFO_MODE_USER (2)
#define FILEINFO_MODE_GROUP (1)
#define FILEINFO_MODE_ALL (0)

#define FILEINFO_ALL_PERM (FILEINFO_MODE_X | FILEINFO_MODE_W | FILEINFO_MODE_R)

typedef int mode_t;

#define filemode(perm, md) ((perm) << (((md) * FILEINFO_MODE_SIZE) + FILEINFO_MODE_BASE))

#define FILEINFO_MODE_MASK (filemode(FILEINFO_ALL_PERM, FILEINFO_MODE_USER) |\
                            filemode(FILEINFO_ALL_PERM, FILEINFO_MODE_GROUP) |\
                            filemode(FILEINFO_ALL_PERM, FILEINFO_MODE_ALL))

#endif