#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page_data
{
	struct file* file;
	size_t length;
	off_t offset;
	int off_mag;
};

struct file_page 
{
	struct file* file;
	size_t length;
	off_t off;
	int off_mag;
	size_t bit_idx;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif
