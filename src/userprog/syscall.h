#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

void sys_exit (int);

struct mmap_info{
  mapid_t id;
  struct file* f;
  void* addr;
  int file_size;
  struct list_elem elem;
};

#endif /* userprog/syscall.h */
