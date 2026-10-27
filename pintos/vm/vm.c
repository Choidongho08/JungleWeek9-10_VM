/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "string.h"
#include "threads/mmu.h"
static struct list frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&frame_table);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable, vm_initializer *init, void *aux) 
{
	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* 할 일: 페이지 생성, VM 타입에 따라 초기화값 가져오기,
		 * TODO: 그 다음 uninit_new를 호출하여 "uninit" 페이지 구조체를 생성합니다.
		 * TODO: uninit_new 호출 후 필드를 수정해야 합니다. */
		
		struct page* page = malloc(sizeof(struct page));
		if(type == VM_ANON)
		{
			uninit_new(page, upage, init, type, aux, anon_initializer);
		}
		if(type == VM_FILE)
		{
			uninit_new(page, upage, init, type, aux, file_backed_initializer);
		}
		// if문으로 vm_type마다 초기화 함수 선택.
		
		/* 페이지 구조체를 만든 뒤에는 해당 프로세스의 보조 페이지 테이블에 삽입하세요. 
		vm.h의 VM_TYPE 매크로를 사용하면 편리할 수 있습니다.*/
		
		/* 할 일: 페이지를 spt에 삽입한다. vm.h의 VM_TYPE 매크로를 사용 */
		spt_insert_page(&thread_current()->spt.spt_hash, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) 
{
	struct page *result = NULL;
	/* TODO: Fill this function. */

	// for문 돌면서 va와 같은 va를 갖고있는 page 찾으면 될 거 같아요 (없으면 NULL)
	struct hash_iterator it;
	struct hash_elem* elem;
	for(hash_first(&it, &spt->spt_hash); elem != NULL; elem = hash_next(&it))
	{
		struct page* page = hash_entry(elem, struct page, spt_hash_elem);
		if(page->va == va)
		{
			result = page;
		}
	}

	return result;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt, struct page *page) 
{
	int succ = false;
	/* TODO: Fill this function. */

	// spt에 page를 넣는 함수. (이미 해당 va가 있으면 false)
	if(spt_find_page(spt, page->va))
		return succ;

	hash_insert(&spt->spt_hash, &page->spt_hash_elem);
	succ = true;

	return succ;
}

void spt_remove_page (struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page (page);
	return true;
}

/* 해제될 구조체 프레임을 가져옵니다. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	/* TODO: 페이지 해제 정책 알아서 정하기
	 * 지금은 FIFO 방식으로 구현해둠. 추후 clock 
	*/
	
	victim = list_front(&frame_table);


	return victim;
}

/* 한 페이지를 이동시킨 후 해당 프레임을 반환합니다.
 * 오류 발생 시 NULL 반환.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	if (!victim) {
		return NULL;
	}
	// 해체될 프레임을 리스트에서 제거하고 swap out 해준다.
	
	pml4_clear_page(thread_current()->pml4, victim->page->va);
	list_remove(victim->elem);
	swap_out(victim->page);
	
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

	void* kva = palloc_get_page(PAL_USER);
	
	if(!kva)
	{
		frame = vm_evict_frame();
	}
	else
	{
		frame = malloc(sizeof(struct frame));
	}

	frame->kva = kva;
	frame->page = NULL;
	list_push_back(&frame_table, frame->elem);

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	/* 주소 존재하지 않거나, 커널 가상 메모리를 가리키거나, 읽기 전용 페이지에 쓰려는 접근이면 페이지 폴트 */
	if (not_present || !user || write) {
		return false;
	}
	
	/* 폴트가 발생한 페이지를 SPT에서 찾습니다.*/
	page = spt_find_page(spt, addr);
	if (!page) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* 할 일: 페이지 테이블 항목을 삽입하여 페이지의 VA를 프레임의 PA에 매핑합니다.
	 * FIFO 를 통해 구현 */
	pml4_set_page(&thread_current()->pml4, page->va, frame->kva, true);
	list_push_back(&frame_table, frame->elem);

	return swap_in (page, frame->kva);
}

uint64_t spt_hash_hash_func(const struct hash_elem *e, void *aux)
{
	const struct page* p = hash_entry(e, struct page, spt_hash_elem);
    return hash_bytes(&p->va, sizeof p->va);
}

bool spt_hash_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
	struct page* a_page = hash_entry(a, struct page, spt_hash_elem);
	struct page* b_page = hash_entry(b, struct page, spt_hash_elem);

	return a_page->va < b_page->va;
}

/* Initialize new supplemental page table */
void supplemental_page_table_init (struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->spt_hash, spt_hash_hash_func, spt_hash_less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy (struct supplemental_page_table *dst, struct supplemental_page_table *src) 
{
	// hash 복사
	dst->spt_hash.elem_cnt = src->spt_hash.elem_cnt;
	dst->spt_hash.bucket_cnt = src->spt_hash.bucket_cnt;
	dst->spt_hash.hash = src->spt_hash.hash;
	dst->spt_hash.less = src->spt_hash.less;
	dst->spt_hash.aux = src->spt_hash.aux;
	dst->spt_hash.buckets = malloc(sizeof dst->spt_hash.buckets * dst->spt_hash.bucket_cnt);

	struct hash_iterator it;
	struct hash_elem* src_elem;
	// for(hash_first(&it, &src->spt_hash); src_elem != NULL; src_elem = hash_next(&it))
	// {
	// 	struct page* src_page = hash_entry(src_elem, struct page, spt_hash_elem);
	// 	vm_alloc_page(page_get_type(src_page), src_page->va, )
	// }
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
