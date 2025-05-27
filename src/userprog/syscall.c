#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

#include "devices/shutdown.h"
#include "devices/input.h"

#include "process.h"

#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
struct file * fd_to_file(int fd);

struct lock syscall_lock;

void
syscall_init (void) 
{
    lock_init(&syscall_lock);
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
  int *syscall_number = (int *)f->esp;
  //printf("System call number: %d\n", syscall_number);

  if (!is_user_vaddr(syscall_number))
  {
    exit(-1);
  }

  switch (*syscall_number)
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
                     if (!is_user_vaddr(cmd_line))
                     {
                       exit(-1);
                     }
                     f->eax = exec(cmd_line);
                     break;
                   }

    case SYS_WAIT: {
                     tid_t pid = *(tid_t *) get_arg(f, 1);
                     f->eax = wait(pid);
                     break;
                   }

    case SYS_CREATE: {
                       const char *file = *(char **) get_arg(f, 4);
                       if (!is_user_vaddr(file))
                       {
                         exit(-1);
                       }
                       unsigned initial_size = *(unsigned *) get_arg(f, 5);
                       f->eax = create(file, initial_size);
                       break;
                     }

    case SYS_REMOVE: {
                       const char *file = *(char **) get_arg(f, 1);
                       if (!is_user_vaddr(file))
                       {
                         exit(-1);
                       }
                       f->eax = remove(file);
                       break;
                     }

    case SYS_OPEN: {
                     const char *file = *(char **) get_arg(f, 1);
                     if (!is_user_vaddr(file))
                     {
                       exit(-1);
                     }
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
                      if (!is_user_vaddr(buffer))
                      {
                       exit(-1);
                      }
                      unsigned size = *(unsigned *) get_arg(f, 3);
                      f->eax = read(fd, buffer, size);
                      break;
                   }

    case SYS_WRITE: {
                      int fd = *(int *) get_arg(f, 5);
                      const void *buffer = *(void **) get_arg(f, 6);
                      if (!is_user_vaddr(buffer))
                      {
                        exit(-1);
                      }
                      unsigned size = *(unsigned *) get_arg(f, 7);
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
                    printf("Unknown system call: %d\n", *syscall_number);
                    exit(-1);
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
    struct info_child* child = cur->info;
      child->used=0;
      if (parent->id_wait == cur->tid){
	child->exit_code = status;
        sema_up(&parent->children_sema);
 	}	
    }
  thread_exit();
} 

tid_t exec(const char * name)
{
  lock_acquire(&syscall_lock);
  tid_t id = process_execute(name);
  lock_release(&syscall_lock);
  return id;
}

int wait(tid_t child)
{
  return process_wait(child);
}

bool create (const char * name, unsigned size)
{
  if(!name)
    return -1;

  lock_acquire(&syscall_lock);
  int error = filesys_create(name, size);
  lock_release(&syscall_lock);

  return error;
}

bool remove(const char * name)
{
  if(!name)
    return -1;

  lock_acquire(&syscall_lock);
  int error = filesys_remove(name);
  lock_release(&syscall_lock);

  return error;
}

int open (const char * fn)
{

  lock_acquire(&syscall_lock);
  struct file * f = filesys_open(fn);
  lock_release(&syscall_lock);

  if(!f)
    return -1;

  struct file_desc *fd = malloc(sizeof(struct file_desc));
  fd->file = f;
  struct thread *t = thread_current();
  t->next_fd++;
  fd->fd = t->next_fd;
  list_push_front(&t->open_files, &fd->elem);

  return fd->fd;
}

int filesize (int fd)
{
  struct file *f = fd_to_file(fd);
  if(!f)
    return -1;

  lock_acquire(&syscall_lock);
  int size = file_length(f);
  lock_release(&syscall_lock);
  return size;
}

int read (int fd, void * buf, unsigned size)
{
  if(!buf)
    return -1;

  unsigned byte_read = 0;

  if(fd == STDIN_FILENO)
  {
    while(byte_read < size)
    {
      uint8_t *cur = (uint8_t *)buf + byte_read;
      *cur = input_getc();
      byte_read++;
    }
    return byte_read;
  }

  struct file *f = fd_to_file(fd);
  if(!f)
    return -1;

  lock_acquire(&syscall_lock);
  byte_read = file_read(f, buf, size);
  lock_release(&syscall_lock);

  return byte_read;
}

int write (int fd, const void * buf, unsigned size)
{
  if(fd == STDOUT_FILENO)
  {
    putbuf(buf, size);
    return size;
  }

  struct file *f = fd_to_file(fd);
  if(!f)
    return -1;
  int bytes_written = 0;
  lock_acquire(&syscall_lock);
  bytes_written = file_write(f, buf, size);
  lock_release(&syscall_lock);

  return bytes_written;
}

void seek(int fd, unsigned pos)
{
  struct file *f = fd_to_file(fd);
  if(!f)
    return;

  lock_acquire(&syscall_lock);
  file_seek(f, pos);
  lock_release(&syscall_lock);
}

unsigned tell (int fd)
{
  struct file *f = fd_to_file(fd);
  if(!f)
    return -1;
  lock_acquire(&syscall_lock);
  unsigned pos = file_tell(f);
  lock_release(&syscall_lock);
  return pos;
}

void close (int fd)
{
  struct thread * t = thread_current();
  if(list_empty(&t->open_files))
    return;

  struct list_elem *e = list_begin(&t->open_files);

  while(e != list_tail(&t->open_files))
  {
    struct file_desc *file_d = list_entry(e, struct file_desc, elem);
    if(file_d->fd == fd)
    {
      file_close(file_d->file);
      list_remove(e);
      free(file_d);
      return;
    }
    e = list_next(e);
  }
}

struct file * fd_to_file(int fd)
{
  struct thread * t = thread_current();
  if(list_empty(&t->open_files))
    return NULL;

  struct list_elem *e = list_begin(&t->open_files);

  while(e != list_tail(&t->open_files))
  {
    struct file_desc file_d = *list_entry(e, struct file_desc, elem);
    if(file_d.fd == fd)
      return file_d.file;

    e = list_next(e);
  }

  return NULL;
}
