#pragma once

struct spin_lock {
    int users;
    int pass;
};

typedef struct spin_lock spin_lock_t;

void lock(spin_lock_t *spin_lock);
void unlock(spin_lock_t *spin_lock);
// critical section
void start_no_irq();
void end_no_irq();