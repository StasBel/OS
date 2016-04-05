#include "lock.h"
#include "util.h"

static volatile int depth = 0;

void lock() {
    cli();
    __sync_fetch_and_add(&depth, 1);
}

void unlock() {
    if (__sync_sub_and_fetch(&depth, 1) == 0) {
        sti();
    }
}


