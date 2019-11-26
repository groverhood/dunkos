#ifndef DUNKOS_ALGO_H
#define DUNKOS_ALGO_H

#ifndef __cplusplus

#define max(a, b) \
   ({ const __typeof__(a) _a = (a); \
      const __typeof__(b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a, b) \
   ({ const __typeof__(a) _a = (a); \
      const __typeof__(b) _b = (b); \
     _a < _b ? _a : _b; })

#endif

#define isnegative(i, size) ({ const __typeof__(i) _i = i; \
                         !(_i ^ (1 << (size * 8 - 1))); })

#define iisnegative(i) (isnegative(i, sizeof i))

#define _name(x) #x
/* Get stringified version of something, including a macro. */
#define name(x) _name(x)

#define halt() __asm__ volatile ("hlt")

#endif