#pragma once

#include "ioport.h"

#define cli() __asm__ volatile ("cli" : : : "cc")
#define sti() __asm__ volatile ("sti" : : : "cc")
#define barrier() __asm__ volatile ("" : : : "memory")

#define	assert(e) \
    if (!(e)) printf("WRONG\n")
