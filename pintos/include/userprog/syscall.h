#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdint.h>

extern struct lock file_lock;

void syscall_init (void);
void sys_exit (uint64_t rdi);

#endif /* userprog/syscall.h */
