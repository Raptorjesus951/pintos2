#include "devices/shutdown.h"
#include "devices/input.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"

#define STDIN 0
#define STDOUT 1
#define STDERR 2

static void syscall_handler (struct intr_frame *);

static void check_user (const uint8_t *uaddr);
static int32_t get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);
static int memread_user (void *src, void *des, size_t bytes);

static struct file_desc* find_file_desc(struct thread *, int fd);

void sys_halt (void);
void sys_exit (int);
pid_t sys_exec (const char *cmdline);
int sys_wait (pid_t pid);

bool sys_create(const char* filename, unsigned initial_size);
bool sys_remove(const char* filename);
int sys_open(const char* file);
int sys_filesize(int fd);
void sys_seek(int fd, unsigned position);
unsigned sys_tell(int fd);
void sys_close(int fd);
int sys_read(int fd, void *buffer, unsigned size);
int sys_write(int fd, const void *buffer, unsigned size);

struct lock filesys_lock;

void
syscall_init (void)
{
  lock_init (&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// in case of invalid memory access, fail and exit.
static void fail_invalid_access(void) {
  if (lock_held_by_current_thread(&filesys_lock))
    lock_release (&filesys_lock);

  sys_exit (-1);
  NOT_REACHED();
}

static void
syscall_handler (struct intr_frame *f)
{
  int syscall_number;
  int intsize = 4;
  int ptrsize = 4;
  int fd;
  ASSERT( sizeof(syscall_number) == 4 ); // assuming x86

  // The system call number is in the 32-bit word at the caller's stack pointer.
  memread_user(f->esp, &syscall_number, intsize);


  // Dispatch w.r.t system call number
  // SYS_*** constants are defined in syscall-nr.h
  switch (syscall_number) {
  case SYS_HALT: // 0
    {
      sys_halt();
      NOT_REACHED();
      break;
    }

  case SYS_EXIT: // 1
    {
      int exitcode;
      memread_user(f->esp + 4, &exitcode, intsize);

      sys_exit(exitcode);
      NOT_REACHED();
      break;
    }

  case SYS_EXEC: // 2
    {
      void* cmdline;
	    memread_user(f->esp + 4, &cmdline, ptrsize);

      uint32_t return_code = sys_exec((const char*) cmdline);
      f->eax = return_code;
      break;
    }

  case SYS_WAIT: // 3
    {
      pid_t pid;
  	  memread_user(f->esp + 4, &pid, sizeof(pid));

      uint32_t return_code = sys_wait(pid);
      f->eax = return_code;
      break;
    }

  case SYS_CREATE: // 4
    {
      const char* filename;
      unsigned initial_size;
      bool return_code;
	    int sizek = sizeof(initial_size);
      memread_user(f->esp + 4, &filename, ptrsize);
      memread_user(f->esp + 8, &initial_size, sizek);

      return_code = sys_create(filename, initial_size);
      f->eax = return_code;
      break;
    }

  case SYS_REMOVE: // 5
    {
      const char* filename;
      bool return_code;
      memread_user(f->esp + 4, &filename, ptrsize);

      return_code = sys_remove(filename);
      f->eax = return_code;
      break;
    }

  case SYS_OPEN: // 6
    {
      const char* filename;
      int return_code;
      
      memread_user(f->esp + 4, &filename,ptrsize);

      return_code = sys_open(filename);
      f->eax = return_code;
      break;
    }

  case SYS_FILESIZE: // 7
    {
      int return_code;
      memread_user(f->esp + 4, &fd, intsize);

      return_code = sys_filesize(fd);
      f->eax = return_code;
      break;
    }

  case SYS_READ: // 8
    {
      uint32_t return_code;
      void *buffer;
      unsigned size;
	    int sizek =sizeof(size);
      memread_user(f->esp + 4, &fd, intsize);
      memread_user(f->esp + 8, &buffer, ptrsize);
      memread_user(f->esp + 12, &size, sizek);

      return_code = sys_read(fd, buffer, size);
      f->eax = return_code;
      break;
    }

  case SYS_WRITE: // 9
    {
      uint32_t return_code;
      const void *buffer;
      unsigned size;
	    int sizek2 = sizeof(buffer);
	    int sizek = sizeof(size);
      memread_user(f->esp + 4, &fd, intsize);
      memread_user(f->esp + 8, &buffer, sizek2);
      memread_user(f->esp + 12, &size, sizek);

      return_code = sys_write(fd, buffer, size);
      f->eax = return_code;
      break;
    }

  case SYS_SEEK: // 10
    {
      unsigned position;
	    int sizek = sizeof(position);
      memread_user(f->esp + 4, &fd, intsize);
      memread_user(f->esp + 8, &position, sizek);

      sys_seek(fd, position);
      break;
    }

  case SYS_TELL: // 11
    {
      uint32_t return_code;

      memread_user(f->esp + 4, &fd, intsize);

      return_code = sys_tell(fd);
      f->eax = return_code;
      break;
    }

  case SYS_CLOSE: // 12
    {
      memread_user(f->esp + 4, &fd, intsize);

      sys_close(fd);
      break;
    }


  /* unhandled case */
  default:
    printf("[ERROR!] system call %d is unimplemented!\n", syscall_number);
    // ensure that waiting (parent) process should wake up and terminate.
    sys_exit(-1);
    break;
  }

}

/****************** System Call Implementations ********************/

void sys_halt(void) {
  shutdown_power_off();
}

void sys_exit(int status) {
  struct thread *current = thread_current ();
  printf("%s: exit(%d)\n", current->name, status);

 
  struct process_control_block *pcb = current->pcb;
  if(pcb != NULL) {
    pcb->exited = 1;
    pcb->exitcode = status;
  }

  thread_exit();
}

pid_t sys_exec(const char *cmdline) {
  const uint8_t* cmd = (const uint8_t*) cmdline;
  check_user(cmd);

  lock_acquire (&filesys_lock); // load() uses filesystem
  pid_t pid = process_execute(cmdline);
  lock_release (&filesys_lock);
  return pid;
}

int sys_wait(pid_t pid) {
  return process_wait(pid);
}

bool sys_create(const char* filename, unsigned initial_size) {
  bool return_code;
  const uint8_t* file = (const uint8_t*)filename;
  check_user(file);

  lock_acquire (&filesys_lock);
  return_code = filesys_create(filename, initial_size);
  lock_release (&filesys_lock);
  return return_code;
}

bool sys_remove(const char* filename) {
  bool return_code;
  const uint8_t* file = (const uint8_t*)filename;
  check_user(file);

  lock_acquire (&filesys_lock);
  return_code = filesys_remove(filename);
  lock_release (&filesys_lock);
  return return_code;
}

int sys_open(const char* file) {
  check_user((const uint8_t*) file);

  struct file* file_opened;
  struct file_desc* fd = palloc_get_page(0);
  if (!fd) {
    return -1;
  }

  lock_acquire (&filesys_lock);
  file_opened = filesys_open(file);
  if (!file_opened) {
    palloc_free_page (fd);
    lock_release (&filesys_lock);
    return -1;
  }

  fd->file = file_opened; //file save
  struct thread *current = thread_current();
  struct list* fd_list = &current->file_descriptors;
  struct thread *back;
  bool empty = list_empty(fd_list);
  if ( empty ) fd->id = 3;
  else {
	fd->id = (list_entry(list_back(fd_list),struct file_desc, elem)->id)+1;
	//fd->id = (back->id) + 1;
  }
  list_push_back(fd_list, &(fd->elem));

  lock_release (&filesys_lock);
  return fd->id;
}

int sys_filesize(int fd) {
  struct file_desc* file_d;

  lock_acquire (&filesys_lock);
  file_d = find_file_desc(thread_current(), fd);

  if(file_d == NULL) {
    lock_release (&filesys_lock);
    return -1;
  }

  int ret = file_length(file_d->file);
  lock_release (&filesys_lock);
  return ret;
}

void sys_seek(int fd, unsigned position) {
  lock_acquire (&filesys_lock);
  struct file_desc* file_d = find_file_desc(thread_current(), fd);

  if(file_d && file_d->file) {
    file_seek(file_d->file, position);
  }
  else
    return;

  lock_release (&filesys_lock);
}

unsigned sys_tell(int fd) {
  lock_acquire (&filesys_lock);
  struct file_desc* file_d = find_file_desc(thread_current(), fd);

  unsigned ret;
  if(file_d && file_d->file) {
    ret = file_tell(file_d->file);
  }
  else
    ret = -1;

  lock_release (&filesys_lock);
  return ret;
}

void sys_close(int fd) {
  lock_acquire (&filesys_lock);
  struct file_desc* file_d = find_file_desc(thread_current(), fd);

  if(file_d && file_d->file) {
    file_close(file_d->file);
    list_remove(&(file_d->elem));
    palloc_free_page(file_d);
  }
  lock_release (&filesys_lock);
}

int sys_read(int fd, void *buffer, unsigned size) {
  check_user((const uint8_t*) buffer);
  check_user((const uint8_t*) buffer + size - 1);

  lock_acquire (&filesys_lock);
  int ret;

  if(fd == STDIN) {
    int i;
    for(i = 0; i < size; ++i) {
      if(! put_user(buffer + i, input_getc()) ) {
        lock_release (&filesys_lock);
        sys_exit(-1); // segfault
      }
    }
    ret = size;
  }
  else {
    // read from file
    struct file_desc* file_d = find_file_desc(thread_current(), fd);

    if(file_d && file_d->file) {
      ret = file_read(file_d->file, buffer, size);
    }
    else // no such file or can't open
      ret = -1;
  }

  lock_release (&filesys_lock);
  return ret;
}

int sys_write(int fd, const void *buffer, unsigned size) {
  check_user((const uint8_t*) buffer);
  check_user((const uint8_t*) buffer + size - 1);

  lock_acquire (&filesys_lock);
  int ret;

  if(fd == STDOUT) { // write to stdout
    putbuf(buffer, size);
    ret = size;
  }
  else {
    // write into file
    struct file_desc* file_d = find_file_desc(thread_current(), fd);

    if(file_d && file_d->file) {
      ret = file_write(file_d->file, buffer, size);
    }
    else // no such file or can't open
      ret = -1;
  }

  lock_release (&filesys_lock);
  return ret;
}

/****************** Helper Functions on Memory Access ********************/
static void
check_user (const uint8_t *uaddr) {
  // check uaddr range or segfaults
  int32_t result = get_user(uaddr);
  if( result  == -1)
  	fail_invalid_access();
}

static int32_t
get_user (const uint8_t *uaddr) {
  int result; 
  if (! ((void*)uaddr < PHYS_BASE)) {
      result = -1;
	  return result;
  }

  asm ("movl $1f, %0; movzbl %1, %0; 1:"
      : "=&a" (result) : "m" (*uaddr));
  return result;
}

static bool
put_user (uint8_t *udst, uint8_t byte) {
  if (! ((void*)udst < PHYS_BASE)) {
    return 0;
  }

  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
      : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  bool result = (error_code != -1);
  return result;
}
static int
memread_user (void *src, void *dst, size_t bytes)
{
  int32_t value;
  size_t i,t;
  for(i=0; i<bytes; i++) {
    t = src+i;
    value = get_user(t);
    if(value != -1) {
		*(char*)(dst + i) = value & 0xff;
	}
	else{  
		fail_invalid_access();
	}
  }
  return (int)bytes;
}

/****** Helper Function on File Access ********************/

static struct file_desc*
find_file_desc(struct thread *t, int fd)
{
  ASSERT (t != NULL);

  if (fd < 3) {
    return NULL;
  }

 
  bool empty = list_empty(&t -> file_descriptors);
  if (!empty) {
    struct list_elem *e = list_begin(&t->file_descriptors);
    for(e;e != list_end(&t->file_descriptors); e = list_next(e))
    {
      struct file_desc *desc = list_entry(e, struct file_desc, elem);
      if(desc->id == fd) {
        return desc;
      }
    }
  }

  return NULL;
}
