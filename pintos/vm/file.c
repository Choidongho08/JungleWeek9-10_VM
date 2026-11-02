/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "filesys.h"
#include "threads/vaddr.h"

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

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
		
	if (addr == 0 || length <= 0 || !writable) {
		return;
	}

	// 페이지 갯수만큼 반복문을 돈다. 
	for (int i = 0; i < length; i += PGSIZE) {
		// file의 정보는 aux에 전달해야한다. aux의 값을 채워준다.
		struct vm_entry *aux = malloc(sizeof(struct vm_entry));

		aux->offset = offset + i; // 새로 읽을 다음 부분부터 offset을 설정한다.
		size_t now_read_bytes; 

		if (length - i > PGSIZE) { // 남은 SIZE가 PGSIZE보다 크다면 한 페이지 만큼 읽는다. 
			now_read_bytes = PGSIZE;
		} else { // 남는 SIZE가 작다면 그만큼 읽는다.
			now_read_bytes = length - i;
		}


		aux->read_bytes = now_read_bytes;
		aux->zero_bytes = PGSIZE - now_read_bytes; // 마지막 PAGE에 공백이 남는다면 그만큼 읽는다.
		aux->file = file;

		// 페이지 객체 생성
		if (!vm_alloc_page_with_initializer(VM_TYPE(VM_FILE), addr + i, writable, init(mmap_lazy_load), aux)) {
			return;
		}
	}

	return true;
}	
/* Do the munmap */
void
do_munmap (void *addr) {
}


static bool
mmap_lazy_load (struct page *page, void *aux) 
{
	struct vm_entry *entry = (struct vm_entry*) aux;

	// 오프셋을 통해서 디스크에서 메모리 위치를 찾아.
	file_seek(entry->file, entry->offset);

	// buffer는 적재될 곳이다. 그래서 우리는 disk 안에 넣어야한다. disk는 kva에 위치한다.
	off_t off = file_read(entry->file, page->frame->kva, entry->read_bytes); 
	
	// 물리 메모리에 적재를 해.
	memset(page->frame->kva + entry->read_bytes, 0, entry->zero_bytes);

	free(aux);

	return true;
}