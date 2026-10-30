/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "string.h"
#include "list.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"

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
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable, vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: 페이지를 생성하고, VM 유형에 따라 초기화자(initializer)를 가져온 뒤,
		 * TODO: uninit_new를 호출하여 'uninit' 페이지 구조체를 생성하십시오. 
		 * TODO: uninit_new를 호출한 후에 해당 필드를 수정해야 합니다. */
		struct page* new_page = malloc(sizeof (struct page));

		if(VM_TYPE(type) == VM_ANON)
		{
			uninit_new(new_page, upage, init, type, aux, anon_initializer);
		}
		else if(VM_TYPE(type) == VM_FILE)
		{
			uninit_new(new_page, upage, init, type, aux, file_backed_initializer);
		}

		/* TODO: 페이지를 spt(보조 페이지 테이블)에 삽입하십시오. */
		spt_insert_page(spt, new_page);

		new_page->writable = writable;
		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) 
{
	struct page *result = NULL;
	result = malloc(sizeof(struct page)); 
	/* TODO: Fill this function. */

	result->va = pg_round_down(va); // va의 값이 가상 주소의 시작점을 의미하겠다는 보장이 없다. 그러니 pg_round_down을 통해 시작점으로 맞춰준다.

	struct hash_elem *e;

	e = hash_find(&spt->spt_hash, &result->spt_hash_elem);
	if (!e) {
		return NULL;
	}

	return hash_entry(e, struct page, spt_hash_elem);
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

/* Get the struct frame, that will be evicted. */
static struct frame* vm_get_victim (void) {
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	if(list_empty(&frame_table))
	{
		return NULL;
	}
	
	victim = list_front(&frame_table);

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim = vm_get_victim ();
	if(!victim)
	{
		return NULL;
	}
	/* TODO: swap out the victim and return the evicted frame. */

	list_remove(&victim->elem);
	if(page_get_type(victim->page) == VM_ANON)
	{
		swap_out(victim->page);
	}
	else
	{
		// 우선 destroy하지만 나중에 file이 dirty상태라면 swap_out
		destroy(victim->page);
	}

	return victim;
}

/* palloc() 및 프레임 가져오기. 
 * 사용 가능한 페이지가 없으면 해당 페이지를 제거하고 반환합니다. 
 * 항상 유효한 주소가 반환됩니다. 
 * 즉, 사용자 풀 메모리가 가득 찼을 경우, 이 함수는 사용 가능한 메모리 공간을 확보하기 위해 프레임을 제거합니다
 *공간.*/
static struct frame* vm_get_frame (void) {
	struct frame *frame = malloc(sizeof(struct frame));

	if (!frame) {
		return NULL;
	}

	/* TODO: Fill this function. */
	frame->kva = palloc_get_page(PAL_USER);
	if(!frame->kva)
	{
		frame = vm_evict_frame(); // kva가 할당되지 않았다는 건 유저프로그램에 꽉 찼다는 뜻이니, 하나를 내쫓고 프레임을 할당한다.
	}

	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	/* 하나 이상의 익명 페이지를 할당하여 `addr`에서 더 이상 폴트가 발생하지 않도록 스택 크기를 늘립니다. 
	할당할 때 `addr`를 `PGSIZE`의 배수인 페이지 시작 주소로 내림해야 합니다.
	**이번 프로젝트에서는 스택 크기를 최대 1MB로 제한해야 합니다.*/

	addr = pg_round_down(addr); // `addr`를 `PGSIZE`의 배수인 페이지 시작 주소로 내림

	/* 1. 일단 alloc을 통해 페이지를 만듬
	   2. do_claim을 통해 프레임을 매핑함
	   3. 그리고 스택 크기를 늘림 */

	if(vm_alloc_page(VM_ANON | VM_MARKER_0, addr, true))
	{
		vm_claim_page(addr);
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f, void *addr, bool user, bool write, bool not_present) 
{
	// not_present = M.M에 적재되지 않은 상태면 true, M.M에 적재(매핑)되어 있으나 Fault가 나면 false
	// user = user 모드로 접근했는지
	struct supplemental_page_table *spt  = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	
	if(addr == NULL || is_kernel_vaddr(addr)) 
		return false;

	if(not_present == true){
		/* Stack Growth가 필요한 경우인지 검사 */
		uintptr_t *rsp = NULL;

		if (user) {
			rsp = f->rsp;
		} 
		else {
			rsp = thread_current()->rsp;
		}

		page = spt_find_page (spt, addr);

		// 페이지가 없을 때에는 스택 growth를 해줘야돼. 그때 적합한 주소 값인지 검사해야됨.

		// 1. addr이 스택 최대 크기 안에 있나?
		// 2. addr이 현재 rsp 근처 또는 그 위에 있나?
		// 둘 다 맞으면 스택을 늘린다.
		if(!page && (USER_STACK + (1 << 20)) >= addr && addr >= rsp - 1) {
			vm_stack_growth(addr);
		}
		else {	
			if (VM_TYPE(page->operations->type) == VM_ANON) { 
				struct anon_page *anon_page = &page->anon;

				if (anon_page->is_stacked) {

					/* rsp는 long 형이기 때문에, rsp - 1 을 통해 확인 (그래도 8바이트임) */
					if (addr >= rsp - 1) {
						vm_stack_growth(addr);
					}
				}
			}
		}
		return vm_do_claim_page (page);
	}
	return false;
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
	/* 페이지에 물리 프레임을 할당합니다. 
	먼저 템플릿에 이미 작성되어 있는 vm_get_frame 호출로 프레임을 얻습니다. 
	그다음 MMU를 설정해야 합니다. 즉, 페이지 테이블에 가상 주소에서 물리 주소로의 매핑을 추가합니다. 
	반환값은 작업의 성공 여부를 나타내야 합니다. */
	page = spt_find_page(&thread_current()->spt, va);
	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool vm_do_claim_page (struct page *page) 
{
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;
	
	if (!pml4_set_page(thread_current()-> pml4, page->va, frame->kva, page->writable)) {
		vm_dealloc_page(page); // page를 물리 메모리와 가상 공간에서 전부 지운다.
		return false;
	}
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// list_push_back(&frame_table, &frame->elem);

	return swap_in (page, frame->kva); // 여기서 vm_alloc_page_with_initializer 때 넣었던 vm_type따라서 init실행 및 설정됨
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
	struct hash_iterator it;
	hash_first(&it, &src->spt_hash);
	while (hash_next(&it))
	{
		struct page *src_page = hash_entry (hash_cur(&it), struct page, spt_hash_elem);
		struct page* page = NULL;
		if(VM_TYPE(src_page->operations->type) == VM_ANON)
		{
			if(!vm_alloc_page(VM_ANON, src_page->va, src_page->writable))
			{
				return false;
			}
			page = spt_find_page(dst, src_page->va);
			if(!page)
			{
				return false;
			}
			vm_do_claim_page(page);
			memcpy(page->frame->kva, src_page->frame->kva, PGSIZE);
		}
		else if(VM_TYPE(src_page->operations->type) == VM_FILE)
		{
			if(!vm_alloc_page(VM_FILE, src_page->va, src_page->writable))
			{
				return false;
			}
			page = spt_find_page(dst, src_page->va);
			if(!page)
			{
				return false;
			}
			vm_do_claim_page(page);
			memcpy(page->frame->kva, src_page->frame->kva, PGSIZE);
		}
		else
		{
			page = malloc(sizeof(struct page));
			if(!page)
			{
				return false;
			}
			uninit_new(page, src_page->va, src_page->uninit.init, src_page->uninit.type, NULL, src_page->uninit.page_initializer);
			page->writable = src_page->writable;
			page->uninit.aux = malloc(sizeof(struct vm_entry));
			memcpy(page->uninit.aux, src_page->uninit.aux, sizeof(struct vm_entry));
			hash_insert(&dst->spt_hash, &page->spt_hash_elem);
		}
	}
	return true;
}

 void spt_hash_clear_func(struct hash_elem *e, void *aux)
 {
	struct page *p = hash_entry(e, struct page, spt_hash_elem);
	destroy(p);
 }

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill (struct supplemental_page_table *spt)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->spt_hash, spt_hash_clear_func);
}
