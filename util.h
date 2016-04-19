#pragma once

#include <stddef.h>
#include "ioport.h"

#define cli() __asm__ volatile ("cli" : : : "cc")
#define sti() __asm__ volatile ("sti" : : : "cc")
#define barrier() __asm__ volatile ("" : : : "memory")

static inline size_t strlen(const char *str) {
    size_t res = 0;
    while (str[res] != 0) {
        ++res;
    }
    return res;
}

static inline int strncmp(const char *from, const char *to, size_t len) {
    unsigned int i = 0;
    for (; from[i] == to[i] && from[i] != 0 && i < len; ++i) {
    }
    if (from[i] == 0 && to[i] == 0) {
        return 0;
    }
    if (i == len) {
        return 0;
    }
    if (from[i] < to[i]) {
        return -1;
    } else if (from[i] == to[i]) {
        return 0;
    } else {
        return 1;
    }
}

#define	assert(e) \
    if (!(e)) printf("WRONG\n")
