CC := x86_64-elf-gcc
LD := x86_64-elf-gcc

CFLAGS := -g -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -ffreestanding \
	-mcmodel=kernel -Wall -Wextra -Werror -pedantic -std=c99 \
	-Wframe-larger-than=4096 -Wstack-usage=4096 -Wno-unknown-warning-option
LFLAGS := -nostdlib -z max-page-size=0x1000

ASM := bootstrap.S videomem.S entry.S threading.S
AOBJ := $(ASM:.S=.o)
ADEP := $(ASM:.S=.d)

SRC := backtrace.c time.c interrupt.c i8259a.c stdio.c vsinkprintf.c stdlib.c \
	serial.c console.c string.c ctype.c list.c main.c misc.c balloc.c \
	memory.c paging.c error.c kmem_cache.c lock.c threads.c test_threads.c
OBJ := $(AOBJ) $(SRC:.c=.o)
DEP := $(ADEP) $(SRC:.c=.d)

all: kernel

kernel: $(OBJ) kernel.ld
	$(LD) $(LFLAGS) -T kernel.ld -o $@ $(OBJ)

%.o: %.S
	$(CC) -D__ASM_FILE__ -g -MMD -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

run:
	qemu-system-x86_64 -kernel kernel -serial stdio

-include $(DEP)

.PHONY: clean
clean:
	rm -f kernel $(OBJ) $(DEP)
