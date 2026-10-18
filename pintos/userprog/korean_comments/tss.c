#include "userprog/tss.h"
#include <debug.h>
#include <stddef.h>
#include "userprog/gdt.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "intrinsic.h"

/* 태스크 상태 세그먼트(TSS).
 *
 * TSS는 x86-64의 구조체로, 프로세서가 직접 지원하는 멀티태스킹 단위인
 * 태스크를 정의하는 데 사용한다. 하지만 이식성, 속도, 유연성 등의 이유로
 * 대부분의 x86-64 운영체제는 TSS를 거의 사용하지 않으며 여기서도 마찬가지다.
 *
 * 다만 사용자 모드에서 발생한 인터럽트의 스택 전환에는 TSS가 필요하다.
 * 사용자 모드(링 3)에서 인터럽트가 발생하면 프로세서는 현재 TSS의 rsp0 멤버를
 * 확인하여 인터럽트 처리에 사용할 스택을 정한다. 따라서 TSS를 만들고
 * 최소한 이 필드를 초기화해야 하며, 이 파일이 그 작업을 수행한다.
 *
 * 인터럽트 게이트나 트랩 게이트로 인터럽트를 처리할 때 x86-64 프로세서는
 * 다음과 같이 동작한다. 여기서 처리하는 모든 인터럽트가 여기에 해당한다.
 *
 * - 중단된 코드와 인터럽트 핸들러가 같은 링에 있으면 스택을 전환하지 않는다.
 *   커널 실행 중에 발생한 인터럽트가 이 경우이며, TSS 내용은 사용하지 않는다.
 *
 * - 중단된 코드와 핸들러의 링이 다르면 TSS에 지정된 새 링의 스택으로 전환한다.
 *   사용자 공간에서 발생한 인터럽트가 이 경우다. 메모리 손상을 피하려면
 *   이미 사용 중인 스택으로 전환해서는 안 된다. 사용자 공간을 실행 중이므로
 *   현재 프로세스의 커널 스택은 사용 중이 아니며, 이를 항상 사용할 수 있다.
 *   따라서 스케줄러가 스레드를 바꿀 때 TSS의 스택 포인터도 새 스레드의
 *   커널 스택을 가리키도록 바꾼다. 이 호출은 thread.c의 schedule에 있다. */

/* 커널 TSS. */
struct task_state *tss;

/* 커널 TSS를 초기화한다. */
void
tss_init (void) {
	/* 이 TSS는 호출 게이트나 태스크 게이트에서 사용하지 않으므로
	 * 참조되는 일부 필드만 초기화한다. */
	tss = palloc_get_page (PAL_ASSERT | PAL_ZERO);
	tss_update (thread_current ());
}

/* 커널 TSS를 반환한다. */
struct task_state *
tss_get (void) {
	ASSERT (tss != NULL);
	return tss;
}

/* TSS의 링 0 스택 포인터가 스레드 스택의 끝을 가리키도록 설정한다. */
void
tss_update (struct thread *next) {
	ASSERT (tss != NULL);
	tss->rsp0 = (uint64_t) next + PGSIZE;
}
