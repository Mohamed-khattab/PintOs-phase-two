#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/shutdown.h"   //ADDED
#include "filesys/filesys.h" //ADDED
#include "lib/user/syscall.c" //ADDED
#include "threads/vaddr.h" //ADDED
#include "filesys/file.h" // For write function
#include "threads/malloc.h" //to support the use of free()
#include "devices/input.c" //to use input_getc()
#include "filesys/filesys.h" 

#define MAX_ARGS 3

static void syscall_handler (struct intr_frame *);

struct file *get_file(int);
long int get_VA(const void *);
void get_args(struct intr_frame *, int *, int);
void validate_pointer(struct intr_frame *);
void exit_sys(int);
void halt_sys();
int open_sys(char *);
bool remove_sys(char *);
int write_sys(struct intr_frame *);
int read_sys(int, void *, unsigned);
int filesize_sys(int);
void fileseek_sys(int, unsigned);
int filetell_sys(int);
void fileclose_sys(int);

int arg[MAX_ARGS];

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  validate_pointer(f);

  switch (*(int *)f->esp)
  {
    case SYS_HALT: //SYS_HALT
      
      halt_sys();
      break;

    case SYS_EXIT: //SYS_EXIT

      get_args(f, &arg[0], 1);
      arg[0] = get_VA((const void *) arg[0]);
      exit_sys(arg[0]);
      break;

    case SYS_EXEC: //SYS_EXEC
      break;

    case SYS_WAIT: //SYS_WAIT
      break;

    case SYS_CREATE: //SYS_CREATE
      break;

    case SYS_REMOVE: //SYS_REMOVE  

      get_args(f, &arg[0], 1);
      arg[0] = get_VA((void *)arg[0]);
      f->eax = remove_sys((char *)arg[0]);
      break;

    case SYS_OPEN:   //SYS_OPEN

      get_args(f, &arg[0], 1);
      arg[0] = get_VA((const void *)arg[0]);
      f->eax = open_sys((char *)arg[0]);
      break;
    
    case SYS_FILESIZE: //SYS_FILESIZE

      get_args(f, &arg[0], 1);
      f->eax = filesize_sys(arg[0]);
      break;

    case SYS_READ: //SYS_READ

      get_args(f, &arg[0], 3);
      arg[1] = get_VA((void *)arg[1]);
      f->eax = read_sys(arg[0], (void *)arg[1], (unsigned)arg[2]);
      break;

    case SYS_WRITE: //SYS_WRITE

      f->eax = write_sys(f);
      break;

    case SYS_SEEK:  //SYS_SEEK

      get_args(f, &arg[0], 2);
      fileseek_sys(arg[0], (unsigned)arg[1]);
      break;

    case SYS_TELL:  //SYS_TELL

      get_args(f, &arg[0], 1);
      f->eax = filetell_sys(arg[0]);
      break;

    case SYS_CLOSE:  //SYS_CLOSE

      get_args(f, &arg[0], 1);
      fileclose_sys(arg[0]);
      break;
  
    default:
      break;
  }
}


void halt_sys()
{
  shutdown_power_off();
}


void exit_sys(int status) //INCOMPLETE
{
  //int status;
  //status = *(int *)(f->esp+1);
  //status = stat;

  if(status < 0)  status = -1;
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  
  cur -> exit_status = status; 
  thread_exit();
}


bool remove_sys(char *name)
{
  lock_acquire(&file_sys_lock);

  bool success = filesys_remove(name);

  lock_release(&file_sys_lock);
  return success;
}


int open_sys(char *name) //(DONE) INCOMPLETE. the thread should keep a list of open files
{
  char *file_name = name;
  lock_acquire(&file_sys_lock);

  struct file *file_ptr = filesys_open(file_name);
  if(!file_ptr){
    lock_release(&file_sys_lock);
    return -1;
  }

   /* Create a new file descriptor. */
  struct file_desc *fd = malloc(sizeof(struct file_desc));
  if (!fd) {
    lock_release(&file_sys_lock);
    return -1;
  }

  fd -> file = file_name;
  fd -> fd = thread_current()->fd;
  thread_current()->fd++; //thread_current()->fd was initialized to 2 inside start_process()
  list_push_back(&thread_current()->file_list, &fd->elem);
  
  lock_release(&file_sys_lock);

  return fd->fd;
}

int write_sys(struct intr_frame *f)
{
  int fd = *((int*)f->esp + 1);
  void* buffer = (void*)(*((int*)f->esp + 2));
  unsigned size = *((unsigned *)f->esp + 3);

  if (fd == 0) {
    f->eax = -1;
    return;
  }
  if (fd == 1) {
    putbuf(buffer, size);
    f->eax = size;
    return;
  }

  lock_acquire(&file_sys_lock); //declared in filesys.h, initialized in filesys.c inside filesys_intit()
  struct file *file_ptr =  get_file(fd);
  if(!file_ptr){
    lock_release(&file_sys_lock);
    return -1;
  }

  int bytes_written = file_write(file_ptr, buffer, size);
  lock_release(&file_sys_lock);
  
  return bytes_written;
}


int read_sys(int fd, void *buffer, unsigned size)
{
  if(size < 0)  return size;

  lock_acquire(&file_sys_lock);

  if(fd == 0){
    uint64_t read_kb[size];
    read_kb[size] = (uint32_t *)buffer;
    for(unsigned i = 0; i < size; i++){
      read_kb[i] = input_getc();
    }

    return size;
  }

  struct file *file_ptr = get_file(fd);
  if(!file_ptr){
    lock_release(&file_sys_lock);
    return -1;
  }

  int32_t bytes_read = file_read(file_ptr, buffer, size);

  lock_release(&file_sys_lock);

  return bytes_read;
}

int filesize_sys(int fd)
{
  lock_acquire(&file_sys_lock);

  struct file *file_ptr = get_file(fd);

  if(!file_ptr){
    lock_release(&file_sys_lock);
    return -1;
  }

  int32_t size =  file_length(file_ptr);

  lock_release(&file_sys_lock);
  return size;
}


void fileseek_sys(int fd, unsigned pos)
{
  lock_acquire(&file_sys_lock);

  struct file *file_ptr = get_file(fd);
  if(!file_ptr){
    lock_release(&file_sys_lock);
    return -1;
  }

  file_seek(file_ptr, pos);

  lock_release(&file_sys_lock);
}


int filetell_sys(int fd)
{
  lock_acquire(&file_sys_lock);

  struct file *file_ptr = get_file(fd);
  if(!file_ptr){
    lock_release(&file_sys_lock);
    return -1;
  }

  int32_t pos = file_tell(file_ptr);
  lock_release(&file_sys_lock);

  return pos;
}


void fileclose_sys(int fd)
{
  lock_acquire(&file_sys_lock);

  struct thread *t = thread_current();
  struct list_elem *next;
  struct list_elem *e = list_begin(&t->file_list);
  struct file_desc *f_ptr;
  
  for (; e != list_end(&t->file_list); e = next){
    next = list_next(e);
    f_ptr = list_entry(e, struct file_desc, elem);

    if(fd == f_ptr->fd){
      file_close(f_ptr);
      list_remove(&f_ptr->elem);
      free(f_ptr);
    }
  }

  lock_release(&file_sys_lock);
}


void validate_pointer(struct intr_frame *f)
{
  if(!is_user_vaddr(f->esp)     ||  \
     !is_user_vaddr(f->esp + 1) ||  \
     !is_user_vaddr(f->esp + 2) ||  \
     !is_user_vaddr(f->esp + 3) ||  \
     f->esp == NULL){

    exit_sys(-1);
  }
}


void get_args(struct intr_frame *f, int *args, int args_num)
{
  int *ptr;
  for(int i = 0; i < args_num; i++){
    ptr = (int *) f->esp + 1 + i;
    args[i] = *ptr;
  }
}

long int get_VA(const void *vaddr) //get virtual address from physical address
{
  void *ptr;
  ptr = pagedir_get_page(thread_current()->pagedir, vaddr);

  if(!ptr)  exit_sys(-1);

  return (int)ptr;
}

//loop through the list of threads' open files to get the file pointer using the fd
struct file *get_file(int fd) 
{
  struct thread *t = thread_current();
  struct list_elem* next;
  struct list_elem* e = list_begin(&t->file_list);
  struct file_desc *f_ptr;

  for(; e != list_end(&t->file_list); e = next){
    e = list_next(e);
    f_ptr = list_entry(e, struct file_desc, elem);
    
    if(fd == f_ptr->fd) return f_ptr->file;
  }

  return NULL;
}