#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "process.h"

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
  printf("esp: %p, inter_id:%d\n",*esp,inter_id);
  *esp += sizeof(int*);
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
      char *cmd_line = (char *) *esp;
      *esp += sizeof(char *);
      tid_t pid = exec(cmd_line);
      *esp -= sizeof(tid_t);
      *(tid_t *)esp = pid;
      break;
    }

    case SYS_WAIT:
    {
      tid_t id = * (tid_t *)esp;
      *esp += sizeof(tid_t);
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

      seek(arg1, arg2);

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

      close(arg1);
      break;
    }
  }

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

void halt(void)
{
  shutdown_power_off();
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

tid_t exec(const char * name)
{
  return process_execute(name);
}

int wait(tid_t child)
{
  return process_wait(child);
}

bool create (const char * name, unsigned size)
{
  return 0;
}

bool remove(const char * name)
{
  return 0;
}

int open (const char * fn)
{
  return -1;
}

int filesize (int fd)
{
  return -1;
}

int read (int fd, void * buf, unsigned size)
{
  return -1;
}

int write (int fd, const void * buf, unsigned size)
{
  return -1;
}

void seek(int fd, unsigned pos)
{

}

unsigned tell (int fd)
{
  return (unsigned)(-1);
}

void close (int fd)
{

}
