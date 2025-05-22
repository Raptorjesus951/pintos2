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
  }
}
void exit (int status){
  thread_current()->exit_code;
  thread_exit();
}
