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

static void *
get_arg(struct intr_frame *f, int index)
{
  return (void *)((uint32_t *) f->esp + index);
}

static void
syscall_handler(struct intr_frame *f)
{
  int syscall_number = *(int *)f->esp;
  printf("System call number: %d\n", syscall_number);

  switch (syscall_number)
  {
    case SYS_HALT:
      halt();
      break;

    case SYS_EXIT: {
                     int status = *(int *) get_arg(f, 1);
                     exit(status);
                     break;
                   }

    case SYS_EXEC: {
                     const char *cmd_line = *(char **) get_arg(f, 1);
                     f->eax = exec(cmd_line);
                     break;
                   }

    case SYS_WAIT: {
                     tid_t pid = *(tid_t *) get_arg(f, 1);
                     f->eax = wait(pid);
                     break;
                   }

    case SYS_CREATE: {
                       const char *file = *(char **) get_arg(f, 1);
                       unsigned initial_size = *(unsigned *) get_arg(f, 2);
                       f->eax = create(file, initial_size);
                       break;
                     }

    case SYS_REMOVE: {
                       const char *file = *(char **) get_arg(f, 1);
                       f->eax = remove(file);
                       break;
                     }

    case SYS_OPEN: {
                     const char *file = *(char **) get_arg(f, 1);
                     f->eax = open(file);
                     break;
                   }

    case SYS_FILESIZE: {
                         int fd = *(int *) get_arg(f, 1);
                         f->eax = filesize(fd);
                         break;
                       }

    case SYS_READ: {
                     int fd = *(int *) get_arg(f, 1);
                     void *buffer = *(void **) get_arg(f, 2);
                     unsigned size = *(unsigned *) get_arg(f, 3);
                     f->eax = read(fd, buffer, size);
                     break;
                   }

    case SYS_WRITE: {
                      int fd = *(int *) get_arg(f, 1);
                      const void *buffer = *(void **) get_arg(f, 2);
                      unsigned size = *(unsigned *) get_arg(f, 3);
                      f->eax = write(fd, buffer, size);
                      break;
                    }

    case SYS_SEEK: {
                     int fd = *(int *) get_arg(f, 1);
                     unsigned position = *(unsigned *) get_arg(f, 2);
                     seek(fd, position);
                     break;
                   }

    case SYS_TELL: {
                     int fd = *(int *) get_arg(f, 1);
                     f->eax = tell(fd);
                     break;
                   }

    case SYS_CLOSE: {
                      int fd = *(int *) get_arg(f, 1);
                      close(fd);
                      break;
                    }

    default:
                    printf("Unknown system call: %d\n", syscall_number);
                    thread_exit();
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
  if (cur->parent != NULL){
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
