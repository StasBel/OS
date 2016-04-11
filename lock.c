#include "lock.h"
#include "util.h"
#include "threads.h"


void lock(spin_lock_t *spin_lock) {
    const int pass = __sync_fetch_and_add(&spin_lock->users, 1);
    while (spin_lock->pass != pass) {
        barrier();
        run_somebody_else();
    }
    __sync_synchronize();
}

void unlock(spin_lock_t *spin_lock) {
    __sync_synchronize();
    __sync_add_and_fetch(&spin_lock->pass, 1);
}

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


