#include "files.h"
#include "kmem_cache.h"
#include "lock.h"
#include "memory.h"

#define TRUE 1
#define FALSE 0
#define NOTHING -1
#define ROOT_NAME ""
#define SEPARATOR '/'

struct file_desc {
    inode_t *inode;
    uint64_t cur_pos;
    int flags;
    int is_free;
};

typedef struct file_desc file_desc_t;
typedef struct kmem_cache slab_ctl_t;

//------------------------------------------- end of declarations & typedefs -------------------------------------------

static file_desc_t file_descs[MAX_FILES];
static slab_ctl_t *inode_slab_ctl;
static dir_t root;

static DEFINE_SPINLOCK(spin_lock_files);

void setup_file_system() {
    for (int i = 0; i < MAX_FILES; i++) {
        file_descs[i].is_free = TRUE;
    }

    inode_slab_ctl = kmem_cache_create(sizeof(inode_t), 1);

    root = kmem_cache_alloc(inode_slab_ctl);
    root->next = NULL;
    root->child = NULL;
    root->is_dir = TRUE;
    root->name = ROOT_NAME;
}

int get_first_free_id() {
    int index = 0;
    while (index < MAX_FILES && file_descs[index].is_free == FALSE) {
        index++;
    }
    if (index < MAX_FILES) {
        return index;
    } else {
        return -1;
    }
}

void next_dir(char *path, size_t *pos, size_t *new_len) {
    *pos += *new_len;

    if (path[*pos] == SEPARATOR) {
        ++*pos;
    }

    *new_len = 0;
    while (path[*pos + *new_len] != 0) {
        if (path[*pos + *new_len] == SEPARATOR) {
            break;
        }
        ++(*new_len);
    }
}

inode_t *get_clear_inode_with_name(char *name) {
    inode_t *new_node = kmem_cache_alloc(inode_slab_ctl);

    size_t len = strlen(name);
    new_node->name = kmem_alloc(len + 1);
    for (size_t i = 0; i < len; ++i) {
        new_node->name[i] = name[i];
    }
    new_node->name[len] = 0;
    new_node->size = 0;
    new_node->capacity_step = 0; // 1 PAGE_SIZE
    new_node->child = NULL;
    new_node->next = NULL;

    return new_node;
}

inode_t *create_new_file(char *name) {
    inode_t *new_node = get_clear_inode_with_name(name);

    new_node->is_dir = FALSE;
    new_node->file_start = kmem_alloc(PAGE_SIZE);

    return new_node;
}

inode_t *create_new_dir(char *name) {
    inode_t *new_node = get_clear_inode_with_name(name);

    new_node->is_dir = TRUE;
    new_node->file_start = NULL;

    return new_node;
}

inode_t *find_file(char *path, int force, int is_dir) {
    size_t pos = 0;
    size_t len = 0;
    next_dir(path, &pos, &len);

    inode_t *node = root;

    while (len != 0) {
        if (!node->is_dir) {
            return NULL;
        }

        inode_t *child = node->child;
        while (child != NULL && (strlen(child->name) != len || (strncmp(child->name, path + pos, len) != 0))) {
            child = child->next;
        }

        if (!child) {
            if (!force) {
                return NULL;
            } else {
                if (path[pos + len] == 0) {
                    if (is_dir) {
                        child = create_new_dir(path + pos);
                    } else {
                        child = create_new_file(path + pos);
                    }
                    child->next = node->child;
                    node->child = child;
                    return child;
                } else {
                    return NULL;
                }
            }
        }
        node = child;

        next_dir(path, &pos, &len);
    }

    if (force == 2) {
        return NULL;
    }
    return node;
}

int open(char *path, int flags) {
    lock(&spin_lock_files);

    if (!NO_CONTRADICTION(flags)) {
        unlock(&spin_lock_files);
        return -1;
    }

    int id = get_first_free_id();

    int force = 0;
    if (flags & CREATE) {
        ++force;
        if (flags & EXCL) {
            ++force;
        }
    }
    file_descs[id].inode = find_file(path, force, FALSE);

    if (file_descs[id].inode == NULL) {
        unlock(&spin_lock_files);
        return -1;
    }
    file_descs[id].flags = flags;
    file_descs[id].cur_pos = 0;
    if (flags & TRUNC) {
        file_descs[id].inode->size = 0;
    }

    if (flags & APPEND) {
        file_descs[id].cur_pos = file_descs[id].inode->size;
    }
    file_descs[id].is_free = FALSE;

    unlock(&spin_lock_files);
    return id;
}

int correct_filesdesc(int desc) {
    if (desc < 0 || desc >= MAX_FILES) {
        return 0;
    }
    if (file_descs[desc].is_free) {
        return 0;
    }
    return 1;
}

int close(int desc) {
    lock(&spin_lock_files);

    if (!correct_filesdesc(desc)) {
        unlock(&spin_lock_files);
        return -1;
    }
    file_descs[desc].is_free = TRUE;

    unlock(&spin_lock_files);
    return 0;
}

long read(int desc, void *buf, size_t n) {
    lock(&spin_lock_files);

    if (!correct_filesdesc(desc) || (file_descs[desc].flags & WRITE_ONLY)) {
        unlock(&spin_lock_files);
        return -1;
    }

    inode_t *node = file_descs[desc].inode;

    size_t k = 0;

    for (; k < n && file_descs[desc].cur_pos < node->size; k++) {
        ((char *) buf)[k] = *(((char *) node->file_start) + file_descs[desc].cur_pos);
        file_descs[desc].cur_pos++;
    }

    unlock(&spin_lock_files);
    return k;
}

void reallocate(inode_t *node) {
    void *new_page = kmem_alloc((1LLU << (node->capacity_step + 1)) * PAGE_SIZE);

    node->capacity_step++;
    for (size_t i = 0; i < node->size; ++i) {
        ((char *) new_page)[i] = ((char *) node->file_start)[i];
    }

    free_pages(node->file_start, node->capacity_step - 1);
    node->file_start = new_page;
}

long write(int desc, void *buf, size_t n) {
    lock(&spin_lock_files);

    if (!correct_filesdesc(desc) || (file_descs[desc].flags & READ_ONLY)) {
        unlock(&spin_lock_files);
        return -1;
    }

    inode_t *node = file_descs[desc].inode;
    size_t k = 0;

    for (; k < n; k++) {
        if (file_descs[desc].cur_pos == node->size) {
            ++node->size;
        }
        if (file_descs[desc].cur_pos == (1LLU << node->capacity_step) * PAGE_SIZE) {
            reallocate(node);
        }
        *(((char *) node->file_start) + file_descs[desc].cur_pos) = ((char *) buf)[k];
        file_descs[desc].cur_pos++;
    }

    unlock(&spin_lock_files);
    return k;
}

int mkdir(char *path) {
    inode_t *node = find_file(path, 2, TRUE);
    if (node == NULL) {
        return -1;
    } else {
        return 0;
    }
}

dir_t *opendir_inode(inode_t *node) { // inode -> dir
    if (node == NULL || !node->is_dir) {
        return NULL;
    }

    dir_t *res = kmem_alloc(sizeof(dir_t));
    *res = node->child;

    return res;
}

dir_t *opendir_path(char *path) { // path -> dir
    inode_t *node = find_file(path, 0, NOTHING);
    dir_t *res = opendir_inode(node);
    return res;
}

void closedir(dir_t *dir) { // close dir
    kmem_free(dir);
}

inode_t *readdir(dir_t *dir) { //dir -> inode
    lock(&spin_lock_files);

    inode_t *result = *dir;
    if (result == NULL) {
        unlock(&spin_lock_files);
        return NULL;
    }
    *dir = result->next;

    unlock(&spin_lock_files);
    return result;
}

inode_t *readdir_path(char *path) { // path -> inode
    lock(&spin_lock_files);

    dir_t *dir = opendir_path(path);
    inode_t *result = *dir;
    if (result == NULL) {
        unlock(&spin_lock_files);
        return NULL;
    }
    *dir = result->next;
    closedir(dir);

    unlock(&spin_lock_files);
    return result;
}

void print_tree(struct inode *node, int height) {
    for (int i = 0; i < height; ++i) {
        printf("---");
    }

    if (node->is_dir) {
        printf("%s/\n", node->name);

        dir_t *dir = opendir_inode(node);

        inode_t *child = readdir(dir);
        while (child != NULL) {
            print_tree(child, height + 1);
            child = readdir(dir);
        }

        closedir(dir);
    } else {
        printf("%s\n", node->name);
    }
}

void print_all_files() {
    print_tree(root, 0);
}