#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  int* p = f->esp;
  int syscall = *p;

  switch(syscall)
  {
    case SYS_EXIT:
    exit(*(p+1));
    break;
    default:
  	exit(1);
    break;
  }
}
void exit (int status){
  struct thread* cur = thread_current();
  struct thread* parent = cur->parent;
  cur->exit_code = status;
  if (cur->parent == NULL){
      struct thread* child;
      struct list_elem *e;
      for (e = list_begin(&parent->children); e != list_end(&parent->children);e = list_next(e)){
        child = list_entry(e, struct thread, elem);
        if (child->tid == cur->tid)
          break;
      }
      if (e != list_end(&parent->children)){
        cur->used=0;
      
        if (parent->id_wait == cur->tid)
          sema_up(&parent->children_sema);
      }
  }
  thread_exit();
} 
