/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "kernel/bitmap.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);
struct bitmap *swap_table;

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* 익명 페이지 서브시스템을 초기화합니다 */
/* 스왑 슬롯 하나는 디스크 섹터 8개로 구성 */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);
	
	/* page 단위로 disk의 스왑영역에 위치하게 된다. */
	swap_table = bitmap_create(disk_size(swap_disk)/(PGSIZE/DISK_SECTOR_SIZE));
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

// 스왑 디스크에서 익명 페이지를 가져와 디스크의 데이터 내용을 메모리로 읽어옵니다.
// 스왑 디스크에 있는 데이터의 위치는 페이지가 스왑 아웃될 때 페이지 구조체에 저장되어 있어야 합니다.
// 스왑 테이블을 업데이트하는 것을 잊지 마십시오( 스왑 테이블 관리 참조 ).

static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	
	for (int i = 0; i < PGSIZE/DISK_SECTOR_SIZE; i++) {
		disk_read(swap_disk, anon_page->bit_idx * 8 + i, kva + DISK_SECTOR_SIZE * i);
	}

	bitmap_set(swap_table, anon_page->bit_idx, false);
	return true;
}

/* Swap out the page by writing contents to the swap disk. */

// 익명 페이지를 스왑 디스크로 스왑 아웃하려면 메모리의 내용을 디스크로 복사해야 합니다.
// 먼저 스왑 테이블을 사용하여 디스크에서 사용 가능한 스왑 슬롯을 찾은 다음,
// 데이터 페이지를 해당 슬롯에 복사합니다. 데이터의 위치는 페이지 구조체에 저장해야 합니다.
// 디스크에 더 이상 사용 가능한 슬롯이 없으면 커널 패닉을 발생시킬 수 있습니다.

static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	size_t idx;

	anon_page->bit_idx = idx = bitmap_scan_and_flip(swap_table, 0, 1, false);

	if (idx == BITMAP_ERROR) {
		PANIC("BITMAP SCAN PANIC!");
		return false;
	}

	for (int i = 0; i < PGSIZE / DISK_SECTOR_SIZE; i++) {
		disk_write(swap_disk, idx * 8 + i, page->frame->kva + DISK_SECTOR_SIZE * i);
	}

	pml4_clear_page(page->frame->owner_pml4, page->va);	
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
