#ifndef DUNKOS_STDDEF_H
#define DUNKOS_STDDEF_H

#define NULL ((void*)0)

#define offsetof(type, member) ((size_t)((unsigned char *)(&((type *)NULL)->member) - ((unsigned char *)NULL)))

#ifndef __cplusplus
typedef int wchar_t;
#endif
typedef unsigned long size_t;
typedef long ssize_t;
typedef long ptrdiff_t;

#endif