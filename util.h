#pragma once

#include <stddef.h>
#include "ioport.h"
#include "stdio.h"

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

static inline char *align4(char *addr) {
    return ((uint64_t) addr & 3) == 0 ? addr : addr + (4 - ((uint64_t) addr & 3));
}

static inline uint32_t read_int(char *str) {
    uint32_t result = 0;
    for (int i = 0; i < 8; i++) {
        result <<= 4;
        if (str[i] >= '0' && str[i] <= '9')
            result += str[i] - '0';
        else if (str[i] >= 'A' && str[i] <= 'F')
            result += str[i] - 'A' + 10;
        else if (str[i] >= 'a' && str[i] <= 'f')
            result += str[i] - 'a' + 10;
        else
            printf("Incorrect integer: %s\n", str);
    }
    return result;
}

#define	assert(e) \
    if (!(e)) printf("WRONG\n")
