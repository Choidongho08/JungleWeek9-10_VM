#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#include "synch.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

struct donation{
	struct lock *wait_lock;				/* 기다리는 lock */
	struct list donated_list;	 		/* 기부한 스레드들 */
	struct list_elem don_elem;			/* donated_list에 넣을 고리 */
};

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */

	struct fd_status
	{
		int fd;
		struct file* file;
		struct list_elem elem;
	};

	struct child_state
	{
		tid_t tid;
		int refer_cnt;
		struct lock refer_cnt_protect_lock;
		int exit_val;
		bool waited;
		struct semaphore wait_sema;
		struct list_elem elem;
	};

struct user_prog_initd_aux
{
	char *fn_copy;
	struct child_state* state;
};

struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */
	int init_priority;  				/* 원본 priority */

	struct donation donate_box;			/* 도네이션용 lock_wait(대기중인 락), 기부한 스레드 리스트, 리스트에 엮기 위한 elem */
	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */

	int64_t wake_ticks;					/* 깨울 시간 (절대시간) */
#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
	
	struct list file_list;
	int last_fd;
	
	struct child_state* share_parent;
	struct list child_state_list;
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

/* 현재 스레드를 sleep_list에 넣고 block상태로 바꿔줌 timer_sleep에서 사용됨 */
void thread_sleep (int64_t sleep_ticks);

/* sleep_list에서 일어날 시간이된 스레드를 깨워주는 함수 - timer_interrupt에서 사용함 */
void thread_wake_up (int64_t ticks);

/* wake_tick이 더 작은지 큰지 판단하는 함수 sleep_list에 넣을때 정렬해서 넣기 위해 필요함. */
bool check_thread_less (const struct list_elem *a, const struct list_elem *b, void *aux);

/* ready_list 정렬을 우선순위에 따라 맞추도록 돕는 함수 */
bool thread_priority_greater(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

/* lock_release 에서 list_max 로 가장 높은 기부자를 찾을 때 쓰는 비교 함수.
   donated_list 의 고리(don_elem)로 꺼내며, a 가 b 보다 낮으면 true. */
bool donation_priority_less (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

/* 스레드 우선순위 확인 후 스위칭 */
void thread_priority_switch (void);

#endif /* threads/thread.h */
