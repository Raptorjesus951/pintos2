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
syscall_handler (struct intr_frame *f) 
{
  void **esp = &f->esp;
  int inter_id = * (int *)*esp;
  *esp += sizeof(int);
  switch (inter_id)
  {
    case SYS_HALT:
      halt();
      break;

    case SYS_EXIT:
    {
      int arg1 = * (int *)*esp;
      *esp += sizeof(int);
      exit(arg1);
      break;
    }

    case SYS_EXEC:
    {
      char *cmd_line = * (char *) *esp;
      *esp += sizeof(char *);
      pid_t pid = exec(cmd_line);
      *esp -= sizeof(pid_t);
      *(pid_t *)esp = pid;
      break;
    }

    case SYS_WAIT:
    {
      pid_t id = * (pid_t *)esp;
      *esp += sizeof(pid_t);
      int status = wait(id);
      *esp -= sizeof(int);
      * (int *) esp = status;
      break;
    }

    case SYS_CREATE:
    {
      char *arg1 = (char *) *esp;
      *esp += sizeof(char *);

      unsigned arg2 = * (unsigned *) *esp;
      *esp += sizeof(unsigned);

      bool status = create(arg1, arg2);

      *esp -= sizeof(bool);
      * (bool *) esp = status;
      break;
    }

    case SYS_REMOVE:
    {
      char *arg1 = (char *) *esp;
      *esp += sizeof(char *);

      bool status = remove(arg1);

      *esp -= sizeof(bool);
      * (bool *) esp = status;
      break;
    }

    case SYS_OPEN:
    {
      char *arg1 = (char *) *esp;
      *esp += sizeof(char *);

      int status = open(arg1);

      *esp -= sizeof(int);
      * (int *) esp = status;
      break;
    }

    case SYS_FILESIZE:
    {
      int arg1 = * (int *) *esp;
      *esp += sizeof(int);

      int status = filesize(arg1);

      *esp -= sizeof(int);
      * (int *) esp = status;
      break;
    }

    case SYS_READ:
    {
      int arg1 = * (int *) *esp;
      *esp += sizeof(int);

      void *arg2 = (void *) *esp;
      *esp += sizeof(void *);

      unsigned arg3 = * (unsigned *) *esp;
      *esp += sizeof(unsigned);

      int status = read(arg1, arg2, arg3);

      *esp -= sizeof(int);
      * (int *) esp = status;
      break;
    }

  case SYS_WRITE:
    {
      int arg1 = * (int *) *esp;
      *esp += sizeof(int);

      void *arg2 = (void *) *esp;
      *esp += sizeof(void *);

      unsigned arg3 = * (unsigned *) *esp;
      *esp += sizeof(unsigned);

      int status = write(arg1, arg2, arg3);

      *esp -= sizeof(int);
      * (int *) esp = status;
      break;
    }

    case SYS_SEEK:
    {
      int arg1 = * (int *) *esp;
      *esp += sizeof(int);

      unsigned arg2 = * (unsigned *) *esp;
      *esp += sizeof(unsigned);

      void status = seek(arg1, arg2);

      *esp -= sizeof(void);
      * (void *) esp = status;
      break;
    }

    case SYS_TELL:
    {
      int arg1 = * (int *) *esp;
      *esp += sizeof(int);

      unsigned status = tell(arg1);

      *esp -= sizeof(unsigned);
      * (unsigned *) esp = status;
      break;
    }

    case SYS_CLOSE:
    {
      int arg1 = * (int *) *esp;
      *esp += sizeof(int);

      void status = close(arg1);

      *esp -= sizeof(void);
      * (void *) esp = status;
      break;
    }
  }

  printf ("system call!\n");
  thread_exit ();
}
