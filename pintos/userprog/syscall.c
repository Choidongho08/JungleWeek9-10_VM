#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/init.h"

void putbuf (const char *, size_t);
void syscall_entry (void);
void syscall_handler (struct intr_frame *);
static void sys_halt (void);
static int sys_write (int fd, const void *buffer, unsigned size);


/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	
	switch (f->R.rax)
	{
		case SYS_HALT :
			sys_halt();
			break;

		case SYS_WRITE : 
			f->R.rax = sys_write(f->R.rdi, (const void *) f->R.rsi, f->R.rdx);
			break;

		default:		
			break;
	}

	thread_exit ();
}

/*
	1. fd검사
	2. 버퍼영역검사
	3. 버퍼 기본 유효성 검사
		3-1 버퍼 NULL 검사
		3-2 유저 주소 범위 검사
		3-3 페이지 매핑 검사
	4. Console 출력
*/
int
sys_write (int fd, const void *buffer, unsigned size) {
	uintptr_t start_addr;	   /* 정수로 변환한 시작 주소 */
	uintptr_t end_addr;	   	   /* 마지막 바이트 주소 */
	uintptr_t page_addr; 	   /* 현재 검사 중인 페이지 */
	uintptr_t last_page; 	   /* 마지막 검사 대상 페이지 */

	/* 1. fd 검사 */
	if (fd != 1)	   /* 1 일 때 콘솔 출력, 1 아닐때 미구현 */
		return -1;	   /* merge 후 잘못된 유저 포인터는 sys_exit (-1) 로 처리 */

	/* 2. 버퍼 영역 검사 */
	if (size == 0)
		return 0;

	/* == 버퍼 기본 유효성 검사 === */
	/* NULL 검사 */
	if (buffer == NULL) 
		return -1;

	start_addr = (uintptr_t) buffer; 		/* 시작 주소 */
	
	/* 주소 오버플로 검사 */
	if (size - 1 > UINTPTR_MAX - start_addr)
		return -1;
		
	end_addr = start_addr + size - 1;	 	/* 마지막 바이트 주소*/

	
	/* 검사할 페이지 범위 계산 */
	page_addr = (uintptr_t) pg_round_down (buffer);	
	last_page = (uintptr_t) pg_round_down ((void *) end_addr);
	
	/* 유저 주소 범위 검사 */
	if (!is_user_vaddr (buffer) || !is_user_vaddr ((void*) end_addr))
		return -1;

	/* 페이지 매핑 검사 */
	while (page_addr <= last_page) {
		/* 실제 페이지 매핑 여부 검사 */
		if (!pml4_get_page (thread_current()->pml4, (void *) page_addr))
			return -1;
		page_addr += PGSIZE;
	}

	/* Console 출력 */
	putbuf(buffer, size);
	return size;
}

void
sys_halt (void) {
	power_off ();
}

