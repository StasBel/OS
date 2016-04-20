#include "initramfs.h"
#include "stdio.h"
#include "multiboot.h"
#include "balloc.h"
#include "files.h"
#include "memory.h"
#include "util.h"
#include "string.h"

#define MODULE_BIT BIT(3)
#define FILE_FLAGS CREATE | READ_WRITE

static char *begin, *end;

void setup_initramfs() {
    extern const uint32_t mboot_info;
    multiboot_info_t *multiboot_info = (multiboot_info_t *) (uintptr_t) mboot_info;

    if (!(multiboot_info->flags & MODULE_BIT)) {
        printf("No modules from multiboot!");
        return;
    }

    bool found = 0;

    multiboot_module_t *module = (multiboot_module_t *) (uintptr_t) multiboot_info->mods_addr;
    for (uint32_t i = 0; i < multiboot_info->mods_count; i++, module++) {
        if (module->mod_end - module->mod_start >= sizeof(cpio_header_t) &&
            !memcmp((void *) (uintptr_t) module->mod_start, CPIO_HEADER_MAGIC, 6)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Cant't find any module!");
        return;
    }

    //reserve memory for initramfs
    begin = (char *) (uintptr_t) module->mod_start;
    end = (char *) (uintptr_t) module->mod_end;
    balloc_add_region((phys_t) begin, end - begin);
    balloc_reserve_region((phys_t) begin, end - begin);
}

void read_all_initramfs() {
    begin = va((phys_t) begin);
    end = va((phys_t) end);

    while (begin < end) {
        //get info from cpio file
        begin = align4(begin);
        DBG_ASSERT(!memcmp(begin, CPIO_HEADER_MAGIC, 6));
        cpio_header_t *header = (cpio_header_t *) begin;
        uint32_t name_len = read_int(header->namesize);
        uint32_t file_len = read_int(header->filesize);
        begin += sizeof(cpio_header_t);
        if (!memcmp(begin, END_OF_ARCHIVE, strlen(END_OF_ARCHIVE))) {
            return;
        }

        // read file_name
        char file_name[MAX_FILES];
        file_name[0] = '/';
        for (uint32_t i = 0; i < name_len; i++, begin++) {
            file_name[i + 1] = *begin;
        }
        file_name[name_len] = 0;

        // create | open
        int file = -1;
        if (S_ISDIR(read_int(header->mode))) {
            mkdir(file_name);
        } else {
            file = open(file_name, FILE_FLAGS);
        }

        // write
        begin = align4(begin);
        if (file != -1) {
            DBG_ASSERT(write(file, begin, file_len) == file_len);
        }

        begin += file_len;
    }
}