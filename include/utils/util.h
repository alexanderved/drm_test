#ifndef _UTILS_UTIL_H_
#define _UTILS_UTIL_H_

#include <stddef.h>

#define CHANGE_ME 1

#define UNUSED __attribute__((unused))

#define FENCE_EMPTY -1
#define FENCE_NOT_SUPPORTED -4

#define container_of(ptr, type, member) ({                \
    const __typeof__(((type *)0)->member) *__mptr = (ptr);    \
    (type *)((char *)__mptr - offsetof(type, member));     \
})



#endif // _UTILS_UTIL_H_