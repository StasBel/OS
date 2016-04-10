#include "threads.h"
#include "lock.h"
#include "interrupt.h"
#include "paging.h"
#include "util.h"

enum thread_state {
    NOT_STARTED_YET, RUNNING, FINISHED, WAITING_TO_JOIN
};

typedef enum thread_state thread_state_t;

struct thread {
    void *stack_pointer;
    void *stack_start;
    void *ret_val;
    int order;
    thread_state_t state;
};

typedef struct thread thread_t;

struct thread_pool {
    volatile pid_t first_free;
    volatile thread_t threads[MAX_THREADS];
    volatile pid_t next[MAX_THREADS];
    volatile pid_t prev[MAX_THREADS];
};

typedef struct thread_pool thread_pool_t;

struct thread_info {
    uint64_t r15, r14, r13, r12, rbx, rbp; //threading.S
    void *start_thread_address;

    void *(*function_address)(void *);

    void *argument;
};

typedef struct thread_info thread_info_t;

static volatile pid_t current_thread = 1;
static volatile pid_t previous_thread = 1;
static thread_pool_t thread_pool;

void setup_threads() {
    thread_pool.first_free = 2;
    thread_pool.threads[1].state = RUNNING;
    thread_pool.threads[0].state = NOT_STARTED_YET;
    thread_pool.next[1] = thread_pool.prev[1] = 1;
    for (int i = 2; i < MAX_THREADS - 1; ++i) {
        thread_pool.next[i] = i + 1;
    }
}

volatile thread_t *get_free_thread() {
    pid_t first = thread_pool.first_free;
    thread_pool.first_free = thread_pool.next[first];
    pid_t next1 = thread_pool.next[1];
    thread_pool.next[first] = next1;
    thread_pool.prev[first] = 1;
    thread_pool.prev[next1] = first;
    thread_pool.next[1] = first;
    return &thread_pool.threads[first];
}

pid_t create_thread(void *(*fptr)(void *), void *arg) {
    lock();
    volatile thread_t *new_thread = get_free_thread();
    new_thread->order = 10;
    new_thread->stack_start = alloc_pages(new_thread->order);
    new_thread->stack_pointer = PAGE_SIZE * (1 << (new_thread->order))
                                + (uint8_t *) new_thread->stack_start
                                - sizeof(thread_info_t);
    thread_info_t *init_val = new_thread->stack_pointer;
    init_val->r12 = init_val->r13 = init_val->r14 = init_val ->r15 = init_val->rbx = init_val->rbp = 0;
    extern void *start_thread; //threading.S
    init_val->start_thread_address = &start_thread;
    init_val->function_address = fptr;
    init_val->argument = arg;
    new_thread->state = RUNNING;
    unlock();
    return (pid_t) (new_thread - thread_pool.threads);
}

pid_t get_this_thread() {
    return current_thread;
}

void switch_threads(void **old_stack_pointer, void *new_stack_pointer); //threading.S

void check_if_thread_finished() {
    volatile thread_t *thread = thread_pool.threads + previous_thread;
    if (thread->state == FINISHED && previous_thread != current_thread) {
        free_pages(thread->stack_start, thread->order);
        thread->state = WAITING_TO_JOIN;
    }
}

void run_thread(pid_t thread_id) {
    if (current_thread == thread_id) {
        return;
    }
    thread_t *thread = (thread_t *) thread_pool.threads + thread_id;
    int ot = current_thread;
    current_thread = thread_id;
    previous_thread = ot;
    thread_t *othread = (thread_t *) thread_pool.threads + previous_thread;
    switch_threads(&othread->stack_pointer, thread->stack_pointer);
    check_if_thread_finished();
}

void finish_thread(void *val) {
    lock();
    int ct = get_this_thread();
    volatile thread_t *current_t = thread_pool.threads + ct;
    current_t->state = FINISHED;
    current_t->ret_val = val;
    thread_pool.prev[thread_pool.next[ct]] = thread_pool.prev[ct];
    thread_pool.next[thread_pool.prev[ct]] = thread_pool.next[ct];
    unlock();
    run_somebody_else();
}

void run_somebody_else() {
    lock();
    for (pid_t index = thread_pool.next[current_thread]; ; index = thread_pool.next[current_thread]) {
        if (index == 0 || thread_pool.threads[index].state != RUNNING) {
            continue;
        }
        run_thread(index);
        break;
    }
    unlock();
}


void *join_thread(pid_t thread) {
    while (thread_pool.threads[thread].state != WAITING_TO_JOIN) {
        run_somebody_else();
        barrier();
    }
    lock();
    void *ret_val = thread_pool.threads[thread].ret_val;
    thread_pool.threads[thread].state = NOT_STARTED_YET;
    thread_pool.next[thread] = thread_pool.first_free;
    thread_pool.first_free = thread;
    unlock();
    return ret_val;
}

