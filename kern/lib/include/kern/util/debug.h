#ifndef DUNKOS_KERN_DEBUG_H
#define DUNKOS_KERN_DEBUG_H

void panic(const char *msg, const char *file, int line);

#ifdef _DEBUG
#define assert(cond) if (!(cond)) panic("Assertion failed: " #cond, __FILE__, __LINE__)
#else
#define assert(cond) (void)0
#endif

#endif