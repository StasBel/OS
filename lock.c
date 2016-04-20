#include "lock.h"

static volatile int depth = 0;

void start_no_irq() {
    cli();
    __sync_fetch_and_add(&depth, 1);
}

void end_no_irq() {
    if (__sync_sub_and_fetch(&depth, 1) == 0) {
        sti();
    }
}


