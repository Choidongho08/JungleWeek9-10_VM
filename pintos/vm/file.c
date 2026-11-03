/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "threads/malloc.h"

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
	
	if (pml4_is_dirty(thread_current()->pml4, page->va)) { // file이 변경 되었는지 확인한다
		// file의 변경을 쓴다.
		file_write_at(page->file.file, page->frame->kva, page->file.read_bytes,page->file.offset); 
	}

	pml4_clear_page(thread_current()-> pml4,page->va); // 가상 주소와 물리 주소 매핑 해제
	
	if (page->frame != NULL) {
		list_remove(&page->frame->elem);
		palloc_free_page(page->frame->kva);
		page->frame = NULL;
	}
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
		
	if (addr == 0 || length <= 0 || !writable) {
		return;
	}

	// 페이지 반복 횟수를 기록한다.
	int cnt = length / PGSIZE;
	if (length % PGSIZE > 0) cnt += 1;

	// 페이지 갯수만큼 반복문을 돈다. 
	for (int i = 0; i < length; i += PGSIZE) {
		// file의 정보는 aux에 전달해야한다. aux의 값을 채워준다.
		struct file_page *aux = malloc(sizeof(struct file_page));

		size_t now_read_bytes; 

		if (length - i > PGSIZE) { // 남은 SIZE가 PGSIZE보다 크다면 한 페이지 만큼 읽는다. 
			now_read_bytes = PGSIZE;
		} else { // 남는 SIZE가 작다면 그만큼 읽는다.
			now_read_bytes = length - i;
		}

		aux->offset = offset + i; // 새로 읽을 다음 부분부터 offset을 설정한다.
		aux->read_bytes = now_read_bytes;
		aux->zero_bytes = PGSIZE - now_read_bytes; // 마지막 PAGE에 공백이 남는다면 그만큼 읽는다.
		aux->file = file;
		aux->page_cnt = cnt;

		// 페이지 객체 생성
		if (!vm_alloc_page_with_initializer(VM_TYPE(VM_FILE), addr + i, writable, mmap_lazy_load, aux)) {
			return;
		}
	}

	return true;
}	
/* Do the munmap */
void
do_munmap (void *addr) {
	// 현재 addr의 페이지를 가져옴
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	
	if (!page) {
		return;
	}
	/* 프로세스가 수정한 모든 페이지만 파일에 다시 기록
	 * 실제 내용(변경된 정보)은 프레임에, 원래 파일 위치 정보는 file_page에 */
	int cnt = page->file.page_cnt;
	for (int i = 0; i < cnt; i++) {
		// 현재 addr의 페이지를 가져옴
		page = spt_find_page(&thread_current()->spt, addr + i * PGSIZE);
		file_backed_destroy(page); // 파일 기반 페이지를 제거한다.
	}
}

static bool
mmap_lazy_load (struct page *page, void *aux) 
{
	struct file_page *entry = (struct file_page*) aux;

	// file 초기화 후 연결
	file_backed_initializer(page, VM_TYPE(VM_FILE), page->frame->kva);

	struct file_page *file_page = &page->file;
	file_page->file = entry->file;
	file_page->offset = entry->offset;
	file_page->read_bytes = entry->read_bytes;
	file_page->zero_bytes = entry->zero_bytes;
	file_page->page_cnt = entry->page_cnt;

	page->frame->kva = entry->file;
	memset(page->frame->kva + entry->read_bytes, 0, entry->zero_bytes);

	free(aux);

	return true;
}