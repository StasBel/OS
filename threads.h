#pragma once

#define MAX_THREADS (1 << 12)

typedef int pid_t;

void setup_threads();
pid_t create_thread(void *(*fptr)(void *), void *arg);
pid_t get_this_thread();
void run_somebody_else();
void join_thread(pid_t thread, void **retval);