#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"

void syscall_init (void);

void sys_exit (int);

typedef int mapid_t;

struct mmap_info{
  mapid_t id;
  struct file* f;
  void* addr;
  int file_size;
  struct list_elem elem;
};

#endif /* userprog/syscall.h */
