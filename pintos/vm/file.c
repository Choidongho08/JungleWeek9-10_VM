/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "string.h"
#include "threads/malloc.h"
#include "threads/mmu.h"
#include "userprog/syscall.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

static bool file_page_initializer(struct page* page, void* aux)
{
	struct file_page_data data = *(struct file_page_data*)aux;
	
	memset(page->va, 0, PGSIZE);
	file_read_at(data.file, page->va, PGSIZE, data.offset + (data.off_mag * PGSIZE));
	page->file.file = data.file;
	page->file.length = data.length;
	page->file.off = data.offset;
	page->file.off_mag = data.off_mag;
	
	free(aux);
	return true;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) 
{
	if(!addr || pg_ofs(addr) != 0 || length == 0)
	{
		return NULL;
	}
	
	void* temp_addr = addr;
	int page_count = (length + PGSIZE - 1) / PGSIZE;
	for(int i = 0; i < page_count + 1; ++i)
	{
		struct file_page_data* data = malloc(sizeof(struct file_page_data));
		data->file = file;
		data->length = length;
		data->offset = offset;
		data->off_mag = i;

		if(!vm_alloc_page_with_initializer(VM_FILE, temp_addr, writable, file_page_initializer, data))
		{
			return NULL;
		}
		temp_addr += PGSIZE;
	}

	return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) 
{
	struct page* p = spt_find_page(&thread_current()->spt, addr);
	if(!p)
	{
		return;
	}

	int pg_count = (p->file.length + PGSIZE - 1) / PGSIZE;
	for(int i = 0; i < pg_count + 1; ++i)
	{
		void* temp_addr = addr + (PGSIZE * i);
		p = spt_find_page(&thread_current()->spt, temp_addr);
		if(!p)
			return;

		if(p->frame)
		{
			if(pml4_is_dirty(thread_current()->pml4, p->va))
			{
				void* buff = pg_round_down(p->va);
				off_t off = PGSIZE * i;
				lock_acquire(&file_lock);
				file_write_at(p->file.file, buff, PGSIZE, p->file.off + off);
				lock_release(&file_lock);
			}

			pml4_clear_page(thread_current()->pml4, p->va);
			palloc_free_page(p->frame->kva);
		}

		spt_remove_page(&thread_current()->spt, p);
	}
}
