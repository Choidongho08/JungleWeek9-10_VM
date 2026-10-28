/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "string.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

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

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	// 새 페이지가 spt에 존재하지않으면
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *page = malloc(sizeof(page));

		if (type == VM_ANON)
			uninit_new(page, upage, init, type, aux, anon_initializer);

		if (type == VM_FILE)
			uninit_new(page, upage, init, type, aux, file_backed_initializer);

		/* TODO: Insert the page into the spt. */
		spt_insert_page(spt, page);
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

/* Get the struct frame, that will be evicted. */
static struct frame* vm_get_victim (void) {
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

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

	list_remove(victim->elem);
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

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame* vm_get_frame (void) {
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
vm_try_handle_fault (struct intr_frame *f, void *addr, bool user, bool write, bool not_present) 
{
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *page = NULL;
	struct vm_types *type = page_get_type(page);
	struct frame *frame;
	
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	
	// 1. 폴트가 발생한 페이지를 보조 페이지 테이블에서 찾습니다
	//  유효한 메모리 참조라면 해당 항목을 사용하여 페이지에 들어갈 데이터가 어디에 있는지 찾습니다. 
	// 	데이터는 파일 시스템이나 스왑 슬롯에 있을 수 있고, 단순히 0으로 채워야 하는 페이지일 수도 있습니다
	//	해당 주소에 데이터가 존재할 이유가 없거나, 커널 가상 메모리를 가리키거나, 
	//	읽기 전용 페이지에 쓰려는 접근이라면 잘못된 접근입니다.
	// 	잘못된 접근은 프로세스를 종료하고 모든 자원을 해제해야 합니다.
	page = spt_find_page(spt, addr);
	
	if (page == NULL)
		return false;
	
	if (not_present || !user || write)
		return false;
	
	// 2. 페이지를 저장할 프레임을 확보합니다. 
	// 	공유를 구현했다면 필요한 데이터가 이미 프레임에 있을 수 있으므로 해당 프레임을 찾을 수 있어야 합니다
	frame = vm_get_frame();

	// 3. 파일 시스템이나 스왑에서 데이터를 읽거나 0으로 채우는 등의 방법으로 프레임에 내용을 준비합니다. 
	// 	공유를 구현했고 필요한 페이지가 이미 프레임에 있다면 이 단계에서 할 일이 없을 수도 있습니다.
	palloc_get_page(PAL_USER);

	// 4. 폴트가 발생한 가상 주소의 페이지 테이블 항목이 해당 물리 페이지를 가리키도록 설정합니다. 
	// 	threads/mmu.c의 함수를 사용할 수 있습니다.

	if(!user || !write || !not_present)
	{
		return false;
	}

	page = spt_find_page(spt, addr);
	if(!page)
	{
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
static bool vm_do_claim_page (struct page *page) 
{
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	list_push_back(&frame_table, frame->elem);

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
	// hash 복사
	dst->spt_hash.elem_cnt = src->spt_hash.elem_cnt;
	dst->spt_hash.bucket_cnt = src->spt_hash.bucket_cnt;
	dst->spt_hash.hash = src->spt_hash.hash;
	dst->spt_hash.less = src->spt_hash.less;
	dst->spt_hash.aux = src->spt_hash.aux;
	dst->spt_hash.buckets = malloc(sizeof(dst->spt_hash.buckets));
	
	// struct hash {
	// size_t elem_cnt;            /* Number of elements in table. */
	// size_t bucket_cnt;          /* Number of buckets, a power of 2. */
	// struct list *buckets;       /* Array of `bucket_cnt' lists. */
	// hash_hash_func *hash;       /* Hash function. */
	// hash_less_func *less;       /* Comparison function. */
	// void *aux;                  /* Auxiliary data for `hash' and `less'. */
	// };

	// /* A hash table iterator. */
	// struct hash_iterator {
	// struct hash *hash;          /* The hash table. */
	// struct list *bucket;        /* Current bucket. */
	// struct hash_elem *elem;     /* Current hash element in current bucket. */
	// };

}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
