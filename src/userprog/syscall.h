#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h" //ADDED. to fix the error: elem has incomplete type

void syscall_init (void);

struct file_desc {
  struct file *file; // pointer to the open file
  int fd; // file descriptor number
  struct list_elem elem; // list element for thread's file list
};

#endif /* userprog/syscall.h */
