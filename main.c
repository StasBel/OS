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
#include "initramfs.h"

#define HALT while(1)
#define SETUP_END_MESSAGE "FINISH SETUP\n"

void main(void) {
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

    read_all_initramfs();

    printf("INITRAMFS FILE TREE:\n");
    print_all_files();

    printf(SETUP_END_MESSAGE);

    HALT;
}
