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

struct lock file_lock; // loader 및 다른 파일 시스템과 통합 필요.

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
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
	if(file_name != NULL && is_user_vaddr(file_name))
	{
		for(char* i = file_name; ; ++i)
		{
			if(!is_user_vaddr(i) || !pml4_get_page(thread_current()->pml4, i))
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
	if(target_fd == 0 || target_fd == 1)
		return;
	
	for(struct list_elem* fd_elem = list_begin(&thread_current()->file_list); fd_elem != list_end(&thread_current()->file_list); fd_elem = list_next(fd_elem))
	{
		struct fd_status* status = list_entry(fd_elem, struct fd_status, elem);
		if(status->fd == target_fd)
		{
			lock_acquire(&file_lock);
			file_close(status->file);
			lock_release(&file_lock);
			list_remove(fd_elem);
			free(status);
			return;
		}
	}
}

/* The main system call interface */
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
	thread_exit ();
}
