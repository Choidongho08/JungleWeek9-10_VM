#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "userprog/process.h"
#include "filesys/filesys.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* 시스템 콜.
 *
 * 예전에는 리눅스의 int 0x80처럼 인터럽트 핸들러로 시스템 콜을 처리했다.
 * x86-64에서는 제조사가 시스템 콜을 효율적으로 요청할 수 있는 syscall 명령을 제공한다.
 *
 * syscall 명령은 모델별 레지스터(MSR)의 값을 읽어 동작한다.
 * 자세한 내용은 매뉴얼을 참고한다. */

#define MSR_STAR 0xc0000081         /* 세그먼트 선택자 MSR. */
#define MSR_LSTAR 0xc0000082        /* 롱 모드의 SYSCALL 진입 주소. */
#define MSR_SYSCALL_MASK 0xc0000084 /* eflags에 적용할 마스크. */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* syscall_entry가 사용자 스택을 커널 모드 스택으로 교체하기 전에는
	 * 인터럽트 서비스 루틴이 인터럽트를 처리하면 안 된다.
	 * 따라서 FLAG_FL을 마스킹한다. */
	write_msr(MSR_SYSCALL_MASK, FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* 시스템 콜의 주 진입 인터페이스. */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: 여기에 구현한다.
	if(f->R.rax == SYS_EXIT)
	{	
		thread_current()->share_parent->exit_val = f->R.rdi;
		thread_exit();
		return;
	}
	if(f->R.rax == SYS_WAIT)
	{
		f->R.rax = process_wait(f->R.rdi);
		return;
	}
	//if(f->R.rax == SYS_OPEN)
	//{
	//	struct file* file = filesys_open(f->R.rdi);
	//	struct fd_status status;
	//	status.fd = 
	//}
	//if(f->R.rax == SYS_CLOSE)
	//{
//
	//}
	printf ("system call!\n");
	thread_exit ();
}
