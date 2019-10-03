#ifndef DUNKOS_STDDEF_H
#define DUNKOS_STDDEF_H

#define NULL ((void*)0)

#ifndef __cplusplus
typedef int wchar_t;
#endif
typedef unsigned long size_t;
typedef long ssize_t;
typedef long ptrdiff_t;

#endif