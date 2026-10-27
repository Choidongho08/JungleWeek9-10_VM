/* uninit.c: 초기화되지 않은 페이지 구현.
 *
 * 모든 페이지는 uninit 페이지로 생성됩니다. 첫 번째 페이지 폴트가 발생하면,
 * 핸들러 체인은 uninit_initialize(page->operations.swap_in)를 호출합니다.
 * uninit_initialize 함수는 페이지 객체를 초기화하고, 
 * vm_alloc_page_with_initializer 함수에서 전달된 초기화 콜백을 호출하여 
 * 페이지를 특정 페이지 객체(anon, file, page_cache)로 변환합니다.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
void
uninit_new (struct page *page, void *va, vm_initializer *init, enum vm_type type, void *aux, bool (*initializer)(struct page *, enum vm_type, void *)) 
{
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
}

/* 첫 번째 오류 발생 시 페이지 초기화 */
/* 페이지 객체를 초기화하고,  vm_alloc_page_with_initializer 함수에서 전달된 초기화 콜백을 호출하여 
 * 페이지를 특정 페이지 객체(anon, file, page_cache)로 변환합니다.*/
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* 먼저 가져오기, page_initialize가 값을 덮어쓸 수 있음 */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */
	/* uninit->page_initializer (page, uninit->type, kva) 
	 지연 로딩을 안하고 바로 띄워야 하는 경우? load
	 * page_initializer는 anon_initializer을 불러온다. 
	 * anon_initializer는 현재 빈 구조체인 `anon_page`에 필요한 정보를 추가하거나 갱신하는 함수이다.
	 * anon_initializer를 호출하고 기본적인 초기화 갱신이 정상적으로 완료되면 true.
	 * 
	 * init 함수가 존재하면 추가 초기화 함수 (init)을 실행. -> lazy_load_segment임.
	 * init 함수가 존재하지 않는다면 init 미실행. 
		//  &page->uninit->aux;에 aux정보를 저장해야됨.
	 * 
	*/
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

/* Free the resources hold by uninit_page. Although most of pages are transmuted
 * to other page objects, it is possible to have uninit pages when the process
 * exit, which are never referenced during the execution.
 * PAGE will be freed by the caller. */
static void
uninit_destroy (struct page *page) {
	struct uninit_page *uninit UNUSED = &page->uninit;
	/* TODO: Fill this function.
	 * TODO: If you don't have anything to do, just return. */
}
