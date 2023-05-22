# CS 140 - PROJECT 2: USER PROGRAMS - DESIGN DOCUMENT

## GROUP
- Mohamed Khattab (mohamed.e.khattab.0@gmail.com)
- Ali ELneklawy (es-ali.elsayed2024@alexu.edu.eg)
- Abdelrahman Ibrahim (iabdelrhman37@gmail.com)
- Ziad Ragab Elsayed (zyad2100@gmail.com)
- FirstName LastName <email@domain.example>


## ARGUMENT PASSING

### DATA STRUCTURES
- A1: Copy here the declaration of each new or changed `struct` or `struct` member, global or static variable, `typedef`, or enumeration. Identify the purpose of each in 25 words or less.

#### thread.h

struct list child_processe_list : list for all child process.
struct thread* parent_thread : pointer to the parent process;
int child_status : when the parent is waitingon its child process child status is updated.
struct file* executable_file : the current file that the process executes.
 struct semaphore wait_child_sema : the parent uses it to block itself until the child process
finishes its execution.
struct semaphore parent_child_sync_sema; : the parent uses it to block itself until the child
process is loaded.
struct list_elem child_elem : struct for the list of the process’s children.

bool is_child_creation_success : to check if child created or a problem happened to communicate with other functions.
#### thread.c

init all data structures that we added into thread.h

  sema_init(&t->parent_child_sync_sema,0);
  sema_init(&t->wait_child_sema,0);
  list_init(&t->open_file_list);
  list_init(&t->child_processe_list);
  t->parent_thread = running_thread();
  t->child_status = -2;


### ALGORITHMS
- A2: Briefly describe how you implemented argument parsing. How do you arrange for the elements of argv[] to be in the right order? How do you avoid overflowing the stack page?

in function
 tid_t
process_execute (const char *file_name) 

we should parase the input and get the file name to be excuted.
then we create a new thread to excute file name and the parent thread should sleep untill the child finished.
parent and child have the same priority so we should make parent sleep to avoid race condition and if we want to change the priority,
phasae 2 may pass all test but it's sure that the other tests in the rest phases will fail.
we check if the child started successfully or not if not we should return error 
but when this boolean variable is_child_creation_success set true or false ?

in Start_Process fuction

in this function we check success -that already implemented if it happened we set the boolean variable is_child_creation_success its true now.
and we should rehandle the wake and sleep of parent and child by using 
sema_up(&parent->parent_child_sync_sema);
sema_down(&child->parent_child_sync_sema);
we should wake up the parent and make child sleep 
the parent should kill the child to avoid zombies.


### RATIONALE
- A3: Why does Pintos implement strtok_r() but not strtok()?


Strok_r() is an updated version of strok() which is safer. 
since The reason for that is that strok() uses a global variable which is not safe as we knows to
keep track of the string position. 
So using strok() in multiple strings simultaneously may lead to race conditions! and we should  to avoid that.


- A4: In Pintos, the kernel separates commands into an executable name and arguments. In Unix-like systems, the shell does this separation. Identify at least two advantages of the Unix approach.

Allowing the shell to separate the commands will provide a layer of abstraction to the code because the shell is a userprogram,
so the validation and separation are made in the user side rather than the kernel’s.

## SYSTEM CALLS

### DATA STRUCTURES

#### thread.h
```
struct open_file{
   int fd;
   struct file* ptr;
   struct list_elem elem;
};

struct thread
  {
    
    struct list open_file_list;          // list of opened files
    struct list child_processe_list;	 // list of child of the process
    struct thread* parent_thread;        // parent of the process
    bool is_child_creation_success;
    int child_status;
    struct file* executable_file;
    struct semaphore wait_child_sema;
    struct semaphore parent_child_sync_sema;
    int fd_last;
    struct list_elem child_elem;
};

```
#### thread.c
```

  sema_init(&t->parent_child_sync_sema,0);
  sema_init(&t->wait_child_sema,0);
  list_init(&t->open_file_list);
  list_init(&t->child_processe_list);
  t->parent_thread = running_thread();
  t->child_status = -2;
  t->fd_last = 2;
  
```
#### userprog/syscall.c
In this file, various system calls are implemented. These system calls include reading from a file, writing from a file, removing, opening and creating files and so on. A pointer `f' which is a pointer to `struct intr_frame` is mainly used to access the stack and grab the arguemnts. The following structure was added to `struct thread`:
```
struct list file_list;
int fd;
int exit_status;
```
Added the follwoing struct to `syscall.h`:
```
struct file_desc {
  struct file *file; // pointer to the open file
  int fd; // file descriptor number
  struct list_elem elem; // list element for thread's file list
};
```

#### process.c
In `process.c`, the file descriptor `fd` of the currently running thread was initialized to 2 since 0 and 1 are resreved for `STDIN` and `STDOUT`. 


### ALGORITHMS
#### B4:
In the case of a system call that causes a full page (4,096 bytes) of data to be copied from user space into the kernel, the least number of inspections of the page table that might result is one. This assumes that the entire page is contiguous in the memory and mapped properly.

On the other hand, the greatest possible number of inspections of the page table would be 1,024. This would occur if the page is fragmented and scattered across different locations in the memory, requiring multiple table lookups to retrieve all the necessary data.

For a system call that only copies 2 bytes of data, the least number of inspections would still be one, assuming the 2-byte data is within a single page and mapped correctly.

There is room for improvement in these numbers by employing techniques such as demand paging or lazy loading. These techniques allow pages to be loaded into the kernel only when they are actually accessed, rather than copying the entire page upfront. This can reduce the number of inspections of the page table and improve overall performance, especially for larger data sets.

#### exec :

In the process_execute function:

 - It begins by declaring several variables: fn_copy, name, next, and tid. These variables are used to store information and control the execution flow.
 - The function then allocates a page of memory using the palloc_get_page function to store a copy of the file_name parameter. This is done to prevent a race condition between the caller and the load() function, which loads the program into memory.
 - Next, it allocates another page of memory to store the file name itself using palloc_get_page. If either of these allocations fails, the previously allocated pages are freed, and the function returns TID_ERROR to indicate failure.
 - The strlcpy function is called to copy the file_name parameter into fn_copy. This is done to ensure that the original file_name is not modified by any concurrent operations.
 - The function then extracts the executable name from the file_name by using strtok_r. It tokenizes the string using a space (" ") as the delimiter. The first token, which is the executable name, is stored in the name variable.
 - Now, a new thread is created using thread_create. It passes the name (executable name), PRI_DEFAULT (default priority), start_process (entry point of the program), and fn_copy (a copy of the file name) as arguments. The return value, which is the thread ID (tid), is stored in the tid variable.
 - If the thread creation fails (tid is TID_ERROR), the allocated memory is freed, and TID_ERROR is returned.
 - The sema_down function is used to synchronize the parent and child threads. The current thread (parent) waits until the child thread signals that it has completed its initialization. This synchronization is done using the parent_child_sync_sema semaphore associated with the current thread.
 - After synchronization, if the name variable is not NULL, it means that memory was allocated for the name variable. In such cases, the allocated memory is freed using palloc_free_page.
 - If the child thread creation was successful (thread_current()->is_child_creation_success is true), the function returns the thread ID (tid).
 - If the child thread creation was not successful, the function returns TID_ERROR.
```
 char* fn_copy;
  char* name ; 
  char* next ;
  tid_t tid;
  fn_copy = palloc_get_page (0);
  name = palloc_get_page (0);
  if (fn_copy == NULL || name == NULL) {
    palloc_free_page(fn_copy);
    palloc_free_page(name);
    return TID_ERROR;
  }
  strlcpy (fn_copy, file_name, PGSIZE); // it check for null parameter in string.s 
  strlcpy(name , file_name , PGSIZE) ; 
  name = strtok_r(name , " " , &next ) ; 
  tid = thread_create (name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
  {
  	palloc_free_page(name);
    palloc_free_page(fn_copy);
    return TID_ERROR;
  }
  sema_down(&thread_current()->parent_child_sync_sema);
  if (name)
  {
    palloc_free_page(name);
  }
  if (!thread_current()->is_child_creation_success) 
  {
    return TID_ERROR;
  }
  return tid;
```

#### wait :

In the process_wait function:

 - Waits for the child process with the given thread ID to exit.
 - Iterates through the child processes list of the current thread.
 - If a matching thread with the given thread ID is found, it is removed from the list.
 - Signals the child process to wake up by calling sema_up on its parent-child synchronization semaphore.
 - Waits for the child process to finish by calling sema_down on the parent's wait child semaphore.
 - Returns the exit status of the child process.
 - If no matching thread is found, returns -1.
 ```
struct thread* parent = thread_current();
struct thread* child = NULL;
for (struct list_elem* e = list_begin(&parent->child_processe_list); e != list_end(&parent->child_processe_list); e = list_next(e))
{
    struct thread* child_process = list_entry(e, struct thread, child_elem);
    if (child_process->tid == child_tid)
    {
        child = child_process;
        break;
    }
}
if (child != NULL) {
    list_remove(&child->child_elem);
    sema_up(&child->parent_child_sync_sema);
    sema_down(&parent->wait_child_sema);
    return parent->child_status;
}
return -1;
```

#### exit :

 in process_exit Function
   Frees the resources of the current process.

 - Closes all open files in the current process's open file list.
 - Frees the memory allocated for each open file structure.
 - Notifies and releases the parent thread's synchronization semaphore for each child process.
 - Allows write access to the executable file if it exists and closes it.
 - Signals the parent thread by calling sema_up on its wait child semaphore if it exists.
 - Destroys the current process's page directory, switching back to the kernel-only page directory.
 ```
void process_exit(void)
{
  struct thread *cur = thread_current();
  uint32_t *pd;

  while (!list_empty(&cur->open_file_list))
  {
    struct open_file *opened_file = list_entry(list_pop_back(&cur->open_file_list), struct open_file, elem);
    file_close(opened_file->ptr);
    palloc_free_page(opened_file);
  }

  while (!list_empty(&cur->child_processe_list))
  {
    struct thread *child = list_entry(list_pop_back(&cur->child_processe_list), struct thread, child_elem);
    child->parent_thread = NULL;
    sema_up(&child->parent_child_sync_sema);
  }

  if (cur->executable_file != NULL)
  {
    file_allow_write(cur->executable_file);
    file_close(cur->executable_file);
  }

  if (cur->parent_thread != NULL)
    sema_up(&cur->parent_thread->wait_child_sema);
  pd = cur->pagedir;
  if (pd != NULL)
  {
    cur->pagedir = NULL;
    pagedir_activate(NULL);
    pagedir_destroy(pd);
  }
}


```


- B6: Any access to user program memory at a user-specified address can fail due to a bad pointer value. Such accesses must cause the process to be terminated. System calls are fraught with such accesses, e.g., a "write" system call requires reading the system call number from the user stack, then each of the call's three arguments, then an arbitrary amount of user memory, and any of these can fail at any point. This poses a design and error-handling problem: how do you best avoid obscuring the primary function of code in a morass of error-handling? Furthermore, when an error is detected, how do you ensure that all temporarily allocated resources (locks, buffers, etc.) are freed? In a few paragraphs, describe the strategy or strategies you adopted for managing these issues. Give an example.

## SYNCHRONIZATION

- B7: The loading of the new executable is ensured to complete before returning from the process_execute function. This is achieved through the use of synchronization and status tracking.

  Synchronization: The function uses a semaphore named parent_child_sync_sema associated with the parent thread (current thread) to synchronize the parent and child threads. After creating the new thread with thread_create, the parent thread waits by calling sema_down(&thread_current()->parent_child_sync_sema). This causes the parent thread to block until the child thread signals that it has completed its initialization.

  Status tracking: The thread_current()->is_child_creation_success flag is used to track the success or failure of the child thread creation. Before returning from process_execute, the function checks the value of this flag. If it is true, indicating that the child thread creation was successful, the function proceeds. Otherwise, if the flag is false, indicating that the child thread creation failed, the function returns TID_ERROR.
  
  By combining these synchronization and status tracking mechanisms, the code ensures that the process_execute function does not return until the new executable has completed loading. The child thread signals the completion of its initialization through the semaphore, and the success or failure status is communicated to the calling thread via the is_child_creation_success flag.

- B8: When P calls wait(C) before C exits:
        The parent process P calls process_wait with the child's thread ID (TID).
        In process_wait, P iterates over its child process list to find the child with the matching TID.
        If the child process is found, it is removed from the parent's child process list using list_remove.
        The parent process then performs synchronization by calling sema_up on the child's parent_child_sync_sema, indicating that it has finished waiting for the child.
        The parent process subsequently waits for the child process to exit by calling sema_down on its own wait_child_sema.
        Meanwhile, the child process, in its start_process function, signals its initialization completion to the parent process by calling sema_up on the parent's parent_child_sync_sema.
        After the child process exits, it sets its exit status in the parent's child_status variable.
        Finally, the parent process retrieves the child's exit status from child_status and returns it.

    After C exits:
        If P calls wait(C) after C has already exited, the child process will not be found in the parent's child process list in process_wait.
        In such cases, process_wait will return immediately with a return value of -1, indicating that the child is invalid or not a child of the calling process.

    When P terminates without waiting before C exits:
        If P terminates without waiting for C, the operating system takes over the responsibility of reaping the child process.
        The child process becomes an orphan and is typically reassigned to a special process called the init process or assigned to another parent process.
        The operating system ensures that all resources allocated to the terminated process, including the orphaned child process, are freed.

    After C exits and P has already terminated:
        If C exits before P terminates, and P has already exited, the operating system is responsible for cleaning up the resources associated with C.
        The operating system ensures that all resources allocated to terminated processes are freed, even if the parent process has already exited.

  In summary, the code manages synchronization between the parent and child processes using semaphores (parent_child_sync_sema, wait_child_sema). It ensures proper resource management by removing the child process from the parent's child process list and signaling the completion of waiting and child process exit. The code handles various scenarios such as waiting for the child before it exits, handling cases where P terminates without waiting, and ensuring resource cleanup by the operating system.


###  Algorithms
put the function u implemented here create , remove , open .....



## RATIONALE
### B9: Why did you choose to implement access to user memory from the kernel in the way that you did?

### B10: What advantages or disadvantages can you see in your design for file descriptors?

### B11: The default tid_t to pid_t mapping is the identity mapping. If you changed it, what advantages are there to your approach?

## SURVEY QUESTIONS
Answering these questions is optional, but it will help us improve the course in future quarters. Feel free to tell us anything you want--these questions are just to spur your thoughts. You may also choose to respond anonymously in the course evaluations at the end of the quarter.

- In your opinion, was this assignment, or any one of the three problems in it, too easy or too hard? Did it take too long or too little time?
- Did you find that working on a particular part of the assignment gave you greater insight into some aspect of OS design?
- Is there some particular fact or hint we should give students in future quarters to help them solve the problems? Conversely, did you find any of our guidance to be misleading?
- Do you have any suggestions for the TAs to more effectively assist students, either for future quarters or the remaining projects?
- Any other comments?
