#include "kmem_cache.h"
#include "interrupt.h"
#include "memory.h"
#include "serial.h"
#include "paging.h"
#include "stdio.h"
#include "misc.h"
#include "time.h"
#include "threads.h"
#include "files.h"
#include "lock.h"
#include "initramfs.h"

#define STUCK while(1)

void main(void) {
    start_no_irq();

    setup_serial();
    setup_misc();
    setup_ints();
    setup_memory();
    setup_initramfs();
    setup_buddy();
    setup_paging();
    setup_alloc();

    setup_threads();
    setup_time();

    setup_file_system();
    read_initramfs();

    end_no_irq();

    printf("FINISH SETUP\n");

    STUCK;
}
