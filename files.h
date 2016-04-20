#pragma once

#include <stddef.h>
#include <stdint.h>

struct inode {
    char *name;
    int capacity_step;
    uint64_t size;
    struct inode *next;
    struct inode *child;
    void *file_start;
    uint8_t is_dir;
};

typedef struct inode inode_t;
typedef struct inode *dir_t;

#define BIT2(x) (1 << x)

#define READ_ONLY BIT2(0)
#define WRITE_ONLY BIT2(1)
#define READ_WRITE BIT2(2)
#define NO_CONTRADICTION(flags) (((flags & READ_ONLY) != 0) + ((flags & WRITE_ONLY) != 0) \
    + ((flags & READ_WRITE) != 0) == 1)
#define APPEND BIT2(3)
#define CREATE BIT2(4)
#define EXCL BIT2(5)
#define TRUNC BIT2(6)

#define MAX_FILES BIT2(16)

void setup_file_system();

int open(char *path, int flags);

int close(int desc);

long read(int desc, void *buf, size_t n);

long write(int desc, void *buf, size_t n);

int mkdir(char *path);

inode_t *readdir(dir_t *dir); //this

inode_t *readdir_path(char *path); //or this

void print_all_files();