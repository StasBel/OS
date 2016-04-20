#pragma once

#include "util.h"

struct spin_lock {
    void *padding;
};

typedef struct spin_lock spin_lock_t;

#define SPINLOCK_INIT(name)    { 0 }
#define DEFINE_SPINLOCK(name)    spin_lock_t name = SPINLOCK_INIT(name)

static inline void spinlock_init(spin_lock_t *lock) { lock->padding = 0; }

static inline void lock(spin_lock_t *lock) {
    (void) lock;
    cli();
}

static inline void unlock(spin_lock_t *lock) {
    (void) lock;
    sti();
}

// critical section
void start_no_irq();
void end_no_irq();