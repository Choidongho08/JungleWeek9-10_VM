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
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

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

struct lock file_lock; // loader 및 다른 파일 시스템과 통합 필요.

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* syscall_entry가 사용자 스택을 커널 모드 스택으로 교체하기 전에는
	 * 인터럽트 서비스 루틴이 인터럽트를 처리하면 안 된다.
	 * 따라서 FLAG_FL을 마스킹한다. */
	write_msr(MSR_SYSCALL_MASK, FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init(&file_lock);
}

void sys_exit(uint64_t rdi)
{
	thread_current()->share_parent->exit_val = (int)rdi;
	thread_exit();
}

void sys_wait(uint64_t* rax, uint64_t rdi)
{
	*rax = process_wait(rdi);
}

void sys_open(uint64_t* rax, uint64_t rdi)
{
	// 여기서 f->R.rdi (파일 이름)에 접근 가능한지 체크.
	char* file_name = (char*)rdi;
	if(file_name != NULL && file_name < KERN_BASE)
	{
		for(char* i = file_name; ; ++i)
		{
			if(i >= KERN_BASE || !pml4_get_page(thread_current()->pml4, i))
			{
				thread_current()->share_parent->exit_val = -1;
				thread_exit();
				return;
			}
			if(*i == '\0')
				break;
		}
	}
	else
	{
		thread_current()->share_parent->exit_val = -1;
		thread_exit();
		return;
	}
	lock_acquire(&file_lock);
	struct file* file = filesys_open(file_name);
	if(!file)
	{
		lock_release(&file_lock);
		*rax = -1;
		return;
	}
	lock_release(&file_lock);
	struct fd_status* status = malloc(sizeof (struct fd_status));
	if(!status)
	{
		lock_acquire(&file_lock);
		file_close(file);
		lock_release(&file_lock);
		*rax = -1;
		return;
	}
	status->fd = thread_current()->last_fd++;
	status->file = file;
	list_push_back(&thread_current()->file_list, &status->elem);
	*rax = status->fd;
}

void sys_close(uint64_t rdi)
{
	int target_fd = (int)rdi;
}

/* 시스템 콜의 주 진입 인터페이스. */
void
syscall_handler (struct intr_frame *f UNUSED) {
	switch ((SYS_CALL_NAME)f->R.rax)
	{
		case SYS_EXIT:
		{
			sys_exit(f->R.rdi);
		}
		return;
		case SYS_WAIT:
		{
			sys_wait(&f->R.rax, f->R.rdi);
		}
		return;
		case SYS_OPEN:
		{
			sys_open(&f->R.rax, f->R.rdi);
		}
		return;
		case SYS_CLOSE:
		{
			sys_close(f->R.rdi);
		}
		return;
	}
	printf ("system call!\n");
	thread_exit ();
}
