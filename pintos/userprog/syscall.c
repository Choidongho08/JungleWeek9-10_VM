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
#include "threads/synch.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

void putbuf (const char *, size_t);
void syscall_entry (void);
void syscall_handler (struct intr_frame *);
static bool sys_create (const char *file_name, unsigned initial_size);
static bool sys_remove (const char *file_name);
static int sys_filesize (int fd);
static int sys_tell (int fd);
static int sys_read (int fd, void *buffer, unsigned size);
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
		case SYS_HALT:
		{
			sys_halt();
		}
		return;
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
		case SYS_CREATE:
		{
			f->R.rax = sys_create ((const char *) f->R.rdi,
					(unsigned) f->R.rsi);

		}
		return;
		case SYS_REMOVE:
		{
			f->R.rax = sys_remove ((const char *) f->R.rdi);

		}
		return;
		case SYS_OPEN:
		{
			sys_open(&f->R.rax, f->R.rdi);
		}
		return;
		case SYS_FILESIZE:
		{
			f->R.rax = sys_filesize ((int) f->R.rdi);

		}
		return;
		case SYS_TELL:
		{
			f->R.rax = sys_tell ((int) f->R.rdi);
		}
		return;
		case SYS_READ:
		{
			/* 사용자 요청의 FD, 버퍼 주소, 크기를 전달하고 읽은 양을 돌려준다. */
			f->R.rax = sys_read ((int) f->R.rdi, (void *) f->R.rsi,
					(unsigned) f->R.rdx);
		}
		return;
		case SYS_WRITE:
		{
			f->R.rax = sys_write (f->R.rdi, (const void *) f->R.rsi, f->R.rdx);
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

static bool
check_file_name (const char *file_name) {
	if (file_name == NULL)
		return false;

	for (const char *p = file_name; ; p++) {
		if (!is_user_vaddr (p)
				|| pml4_get_page (thread_current ()->pml4, p) == NULL)
			return false;
		if (*p == '\0')
			return true;
	}
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
	{
		sys_exit(-1);
		return -1;	   /* merge 후 잘못된 유저 포인터는 sys_exit (-1) 로 처리 */
	}

	/* 2. 버퍼 영역 검사 */
	if (size == 0)
		return 0;

	/* == 버퍼 기본 유효성 검사 === */
	/* NULL 검사 */
	if (buffer == NULL) 
	{
		sys_exit(-1);
		return -1;
	}

	start_addr = (uintptr_t) buffer; 		/* 시작 주소 */
	
	/* 주소 오버플로 검사 */
	if (size - 1 > UINTPTR_MAX - start_addr)
	{
		sys_exit(-1);
		return -1;
	}
		
	end_addr = start_addr + size - 1;	 	/* 마지막 바이트 주소*/

	
	/* 검사할 페이지 범위 계산 */
	page_addr = (uintptr_t) pg_round_down (buffer);	
	last_page = (uintptr_t) pg_round_down ((void *) end_addr);
	
	/* 유저 주소 범위 검사 */
	if (!is_user_vaddr (buffer) || !is_user_vaddr ((void*) end_addr))
	{
		sys_exit(-1);
		return -1;
	}

	/* 페이지 매핑 검사 */
	while (page_addr <= last_page) {
		/* 실제 페이지 매핑 여부 검사 */
		if (!pml4_get_page (thread_current()->pml4, (void *) page_addr))
		{
			sys_exit(-1);
			return -1;
		}
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

static bool
sys_create (const char *file_name, unsigned initial_size) { // 파일생성
	if (!check_file_name (file_name)) {
		sys_exit (-1);
		return false;
	}

	lock_acquire(&file_lock);//FD
	bool success = filesys_create (file_name, initial_size);

	lock_release(&file_lock);//FD
	return success;
}

static bool
sys_remove (const char *file_name) {
	if (!check_file_name (file_name)) {
		sys_exit (-1);
		return false;
	}

	lock_acquire(&file_lock);//FD
	bool success = filesys_remove (file_name);

	lock_release(&file_lock);//FD
	return success;
}

static int
sys_filesize (int fd) {
	struct file *file = NULL;
	struct list *files = &thread_current ()->file_list;

	for (struct list_elem *elem = list_begin (files);
			elem != list_end (files); elem = list_next (elem)) {
		struct fd_status *status = list_entry (elem, struct fd_status, elem);
		if (status->fd == fd) {
			file = status->file;
			break;
		}
	}
	if (file == NULL)
		return -1;

	lock_acquire (&file_lock);
	int size = file_length (file);
	lock_release (&file_lock);
	return size;
}

static int
sys_tell (int fd) {
	struct file *file = NULL;
	struct list *files = &thread_current ()->file_list;

	for (struct list_elem *elem = list_begin (files);
			elem != list_end (files); elem = list_next (elem)) {
		struct fd_status *status = list_entry (elem, struct fd_status, elem);
		if (status->fd == fd) {
			file = status->file;
			break;
		}
	}
	if (file == NULL)
		return -1;

	lock_acquire (&file_lock);
	int position = file_tell (file);
	lock_release (&file_lock);
	return position;
}

static int
sys_read (int fd, void *buffer, unsigned size) {
	/* 일반 파일을 찾으면 해당 파일의 주소를 저장한다.
	 * FD 0은 표준 입력이므로 이 파일 포인터를 사용하지 않는다. */
	struct file *file = NULL;
	struct list *files = &thread_current ()->file_list;
	uintptr_t start_addr;  /* 버퍼 시작 주소를 정수로 저장 */
	uintptr_t end_addr;    /* 저장할 마지막 바이트의 주소 */
	uintptr_t page_addr;   /* 현재 검사 중인 페이지의 시작 주소 */
	uintptr_t last_page;   /* 마지막 검사 대상 페이지의 시작 주소 */
	uint64_t *pte;        /* 페이지의 연결 정보와 접근 권한이 담긴 항목 */

	/* 음수 FD와 표준 출력에서는 읽을 수 없다. */
	if (fd < 0 || fd == 1)
		return -1;

	/* 표준 입력은 건너뛰고, 일반 파일만 현재 프로세스의 목록에서 찾는다. */
	if (fd != 0) {
		for (struct list_elem *elem = list_begin (files);
				elem != list_end (files); elem = list_next (elem)) {
			/* 목록의 연결 고리에서 FD 번호와 파일 주소가 담긴 항목을 얻는다. */
			struct fd_status *status =
				list_entry (elem, struct fd_status, elem);
			if (status->fd == fd) {
				file = status->file;
				break;
			}
		}
		if (file == NULL)
			return -1;
	}

	/* 0바이트 요청은 버퍼에 접근하거나 입력을 기다리지 않는다. */
	if (size == 0)
		return 0;

	/* 표준 입력과 일반 파일 모두 같은 버퍼 검사를 거친다.
	 * 잘못된 주소는 파일 락을 잡기 전에 프로세스를 종료한다. */
	if (buffer == NULL) {
		sys_exit (-1);
		return -1;
	}

	start_addr = (uintptr_t) buffer;
	/* 마지막 주소 계산이 정수의 최대값을 넘어가는지 먼저 검사한다. */
	if (size - 1 > UINTPTR_MAX - start_addr) {
		sys_exit (-1);
		return -1;
	}
	end_addr = start_addr + size - 1;

	/* 시작과 끝이 모두 사용자 주소 범위 안에 있어야 한다. */
	if (!is_user_vaddr (buffer)
			|| !is_user_vaddr ((void *) end_addr)) {
		sys_exit (-1);
		return -1;
	}

	/* 시작 주소만 검사하지 않고, 버퍼가 걸친 모든 페이지를 검사한다. */
	page_addr = (uintptr_t) pg_round_down (buffer);
	last_page = (uintptr_t) pg_round_down ((void *) end_addr);
	while (page_addr <= last_page) {
		if (pml4_get_page (thread_current ()->pml4,
					(void *) page_addr) == NULL) {
			sys_exit (-1);
			return -1;
		}

		/* 0은 새 매핑을 만들지 않고 기존 관리 항목만 조회한다는 뜻이다.
		 * read는 버퍼에 써 넣으므로 사용자 접근 및 쓰기 권한이 필요하다.
		 * 앞의 NULL 조건이 참이면 뒤의 권한 검사는 실행하지 않는다. */
		pte = pml4e_walk (thread_current ()->pml4, page_addr, 0);
		if (pte == NULL || !is_user_pte (pte) || !is_writable (pte)) {
			sys_exit (-1);
			return -1;
		}
		page_addr += PGSIZE;
	}

	/* 표준 입력은 한 바이트씩 받아 검사한 버퍼에 저장한다.
	 * void 포인터는 직접 인덱싱할 수 없어 바이트 포인터로 바꾼다.
	 * 입력을 기다리는 동안 파일 시스템 락을 잡지 않는다. */
	if (fd == 0) {
		uint8_t *bytes = buffer;
		for (unsigned i = 0; i < size; i++)
			bytes[i] = input_getc ();
		return size;
	}

	/* 일반 파일은 공통 락을 사용해 읽고, 반드시 락을 푼 뒤 반환한다.
	 * file_read가 파일 위치를 실제 읽은 양만큼 자동으로 이동시킨다. */
	lock_acquire (&file_lock);
	int bytes_read = file_read (file, buffer, size);
	lock_release (&file_lock);

	/* 요청 크기보다 적을 수 있으며, 파일 끝에서 더 읽을 내용이 없으면 0이다.
	 * 바이트 읽기이므로 문자열 종료 문자도 따로 붙이지 않는다. */
	return bytes_read;
}
