/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "string.h"
#include "list.h"

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

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: 페이지를 생성하고, VM 유형에 따라 초기화자(initializer)를 가져온 뒤,
		 * TODO: uninit_new를 호출하여 'uninit' 페이지 구조체를 생성하십시오. 
		 * TODO: uninit_new를 호출한 후에 해당 필드를 수정해야 합니다. */
		struct page* new_page = malloc(sizeof (struct page));
		if(type == VM_ANON)
		{
			uninit_new(new_page, upage, init, type, aux, anon_initializer);
		}
		if(type == VM_FILE)
		{
			uninit_new(new_page, upage, init, type, aux, file_backed_initializer);
		}

		/* TODO: 페이지를 spt(보조 페이지 테이블)에 삽입하십시오. */
		hash_insert(&spt->spt_hash, &new_page->spt_hash_elem);
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
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

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
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
