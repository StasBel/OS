#include "elf.h"
#include "files.h"
#include "util.h"
#include "memory.h"
#include "kmem_cache.h"
#include "process.h"

elf_contents * create_elf_contents(){
	elf_contents * ret = kmem_alloc(sizeof(elf_contents));
	list_init(&ret->mm_list_head);
	return ret;
}

void destroy_elf_contents(elf_contents * cont) {
	kmem_free(cont);
}

elf_contents * parse_elf(char * path) {
	elf_contents * ret = create_elf_contents();

	int fd = open(path, READ_ONLY);
	size_t i = 0;
	struct elf_hdr hdr;
	struct elf_phdr phdr;

	read(fd, &hdr, sizeof(struct elf_hdr));
	ret->entry = hdr.e_entry;

	seek(fd, hdr.e_phoff);

	for (i = 0; i < hdr.e_phnum; i++) {
		read(fd, &phdr, sizeof(struct elf_phdr));

		if (phdr.p_type == PT_LOAD) {
			void * buf = kmem_alloc_aligned(phdr.p_memsz, PAGE_SIZE_2M);
			int offset = get_seek_offset(fd);
			seek(fd, phdr.p_offset);
			read(fd, buf, phdr.p_memsz);
			seek(fd, offset);

			p_mm_entry * new_entry = create_p_mm_entry(phdr.p_vaddr, pa(buf), phdr.p_memsz, US_CODE);
			list_add(&new_entry->list, &ret->mm_list_head);
		}
	}

	close(fd);
	return ret;
}
