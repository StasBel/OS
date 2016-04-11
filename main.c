#include "kmem_cache.h"
#include "interrupt.h"
#include "memory.h"
#include "serial.h"
#include "paging.h"
#include "stdio.h"
#include "misc.h"
#include "time.h"
#include "threads.h"
#include "test_threads.h"
#include "lock.h"

void main(void)
{
    start_no_irq();

	setup_serial();
	setup_misc();
	setup_ints();
	setup_memory();
	setup_buddy();
	setup_paging();
	setup_alloc();

    setup_threads();
    local_irq_enable();
    setup_time();

    test_threads();

    printf("FINISH SETUP\n");

	while (1);
}
