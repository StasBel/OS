#include "test_thread.h"
#include "threads.h"
#include "stdio.h"
#include "util.h"
#include "lock.h"

typedef enum {
    NOT_MARKED, MARKED, MARKED2
} state_t;

state_t buffer[MAX_THREADS];

void *fun_finish(void *arg __attribute__((unused))) {
    buffer[get_current_thread()] = MARKED;
    return 0;
}

void test_finish() {
    printf("Start test_finish:\n");
    int arg = 0;
    pid_t thread = create_thread(fun_finish, &arg);
    buffer[thread] = NOT_MARKED;
    run_somebody_else();
    assert(buffer[thread] == MARKED);
    printf("End test_finish:\n");
}

void *fun1_arg(void *arg) {
    assert(*(int *) arg == 1);
    buffer[get_current_thread()] = MARKED;
    run_somebody_else();
    return 0;
}

void *fun2_arg(void *arg) {
    assert(*(int *) arg == 2);
    buffer[get_current_thread()] = MARKED;
    run_somebody_else();
    return 0;
}

void test_arg() {
    printf("Start test_arg:\n");
    int arg1 = 1, arg2 = 2;
    pid_t thread1 = create_thread(fun1_arg, &arg1), thread2 = create_thread(fun2_arg, &arg2);
    buffer[thread1] = NOT_MARKED;
    buffer[thread2] = NOT_MARKED;
    run_somebody_else();
    assert(buffer[thread1] == MARKED);
    assert(buffer[thread2] == MARKED);
    printf("End test_arg:\n");
}

void *fun1_lock(void *arg) {
    pid_t id = get_current_thread();
    buffer[id] = MARKED;
    lock();
    buffer[id] = MARKED2;
    lock();
    *(int *) arg = 1;
    printf("test_lock common_arg:%d\n", *(int *) arg);
    unlock();
    unlock();
    return 0;
}

void *fun2_lock(void *arg) {
    pid_t id = get_current_thread();
    buffer[id] = MARKED;
    lock();
    buffer[id] = MARKED2;
    lock();
    *(int *) arg = 2;
    printf("test_lock common_arg:%d\n", *(int *) arg);
    unlock();
    unlock();
    return 0;
}

void test_lock() {
    printf("Start test_lock:\n");
    int common_arg = 0;
    pid_t thread1 = create_thread(fun1_lock, &common_arg), thread2 = create_thread(fun2_lock, &common_arg);
    buffer[thread1] = NOT_MARKED;
    buffer[thread2] = NOT_MARKED;
    run_somebody_else();
    assert(buffer[thread1] = MARKED);
    assert(buffer[thread2] = MARKED);
    run_somebody_else();
    assert(buffer[thread1] = MARKED2);
    assert(buffer[thread2] = MARKED2);
    printf("End test_lock:\n");
}

void *fun_join(void *arg) {
    const int TIMES = 100;
    for (int i = 0; i < TIMES; ++i) {
        run_somebody_else();
    }
    return arg;
}

void test_join() {
    printf("Start test_join:\n");
    int arg = 5;
    pid_t t1 = create_thread(fun_join, &arg);
    void *retval;
    join_thread(t1, &retval);
    assert(retval == &arg);
    printf("End test_join:\n");
}

void test_threads() {
    test_finish();
    test_arg();
    test_lock();
    test_join();
}