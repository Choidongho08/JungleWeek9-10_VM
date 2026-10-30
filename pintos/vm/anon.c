/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* 익명 페이지 서브시스템을 초기화합니다 */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = NULL;
	
	/* page 단위로 disk의 스왑영역에 위치하게 된다. */

	
}

/* Initialize the file mapping */
/* 먼저 `page->operations`에 익명 페이지의 핸들러를 설정합니다. 
 * 현재 빈 구조체인 `anon_page`에 필요한 정보를 추가하거나 갱신할 수 있습니다. 
 * 이 함수는 익명 페이지(`VM_ANON`)의 초기화 함수입니다.
 *
 *`userprog/process.c`의 `load_segment`와 `lazy_load_segment`를 구현하세요.
 * 실행 파일의 세그먼트를 불러오는 기능을 구현해야 합니다. 
 * 이 페이지들은 모두 커널이 페이지 폴트를 감지했을 때에만 지연 로딩되어야 합니다.
 * 
 * 프로그램 로더의 핵심인 `userprog/process.c`의 `load_segment` 반복문을 수정해야 합니다. 
 * 반복할 때마다 `vm_alloc_page_with_initializer`를 호출하여 로딩 대기 상태의 페이지 객체를 만듭니다. 
 * 실제 파일에서 세그먼트를 불러오는 시점은 페이지 폴트가 발생했을 때입니다.
*/
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 페이지의 오퍼레이션에 익명 페이지의 핸들러를 설정 */
	page->operations = &anon_ops;

	// 페이지를 특정 페이지 객체(anon, file, page_cache)로 변환합니다.
	struct anon_page *anon_page = &page->anon;

	// VM_MARKER_0이라면 is_stacked 를 1로 표시. stack에 접근을 할거라는 뜻
	anon_page->is_stacked = (type & VM_MARKER_0) != 0;

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
