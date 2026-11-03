/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

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
/* 파일 기반 페이지를 초기화하는 함수다.
먼저 page->operations에 파일 기반 페이지를 처리하기 위한 핸들러들을 설정한다.
메모리를 뒷받침하는 파일에 대한 정보 등 페이지 구조체의 일부 정보를 업데이트할 수 있다.
이 함수는 파일 기반 페이지가 생성될 때 해당 페이지가 어떤 파일과 연결되어 있는지 등의 정보를 설정하는 역할을 한다.
*/
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
/* swap_in: 파일에서 데이터를 읽어 메모리에 적재 */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
/* 메모리의 변경 내용을 파일에 기록하는 등의 처리 */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
/* 페이지가 제거될 때 파일 참조를 정리 */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */

/* open()으로 파일 열기 → FD 반환 → mmap()에 FD 전달
→ 파일의 offset부터 length만큼 매핑할 페이지 객체를 SPT에 등록
→ 실제 접근 시 페이지 폴트 발생 → 물리 프레임 확보 및 파일 데이터 적재
→ 페이지 테이블에 가상 주소와 물리 주소 연결. */

// 해당 가상 페이지가 아직 물리 메모리에 없다면 페이지 폴트가 발생.

// 그러면 커널은 다음 작업을 수행해.
// 1. 물리 프레임을 할당한다.
// 2. 파일에서 데이터를 읽는다.
// 3. 물리 프레임에 데이터를 저장한다.
// 4. 가상 주소와 물리 프레임을 페이지 테이블로 연결한다.

/* mmap은 파일 데이터를 가상 주소 공간에 연결하는 것이고,
실제 물리 메모리 적재는 페이지 폴트가 발생했을 때 이루어진다는 점 */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	struct file_page *fp;
	fp->file = file;
	fp->length = 
	vm_alloc_page_with_initializer(VM_FILE, addr, writable, file_backed_initializer ,fp);

	file_read_at(file, kva, length, offset);
}

/* Do the munmap */
void
do_munmap (void *addr) {
}
