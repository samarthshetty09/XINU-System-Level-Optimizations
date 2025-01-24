[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/F76xyLG9)
# Project 3: Demand Paging

## 1. Introduction

Demand paging is a method of mapping a large address space into a relatively
small amount of physical memory. It allows a program to use an address space
that is larger than the physical memory, and access non-contiguous sections of
the physical memory in a contiguous way.
Demand paging is accomplished by using a "backing store" (usually disk) to hold
pages of memory that are not currently in use.

From this point on, only the details of this project are discussed.
It is assumed that you have read the Intel documents and are comfortable with
paging concepts and the Intel-specific details.
Here are some suggested reading materials:

* [Address Translation example for Intel Processors (By: Joe Pfeiffer)][address_translation_example]
* [Intel System Programming Model][intel_system_programming_model]

## 2. Goal

The goal of this project is to implement the following system calls and their
supporting infrastructure (See [paging/xm.c](paging/xm.c),
[paging/vcreate.c](paging/vcreate.c), [paging/vgetmem.c](paging/vgetmem.c),
[paging/policy.c](paging/policy.c) and [paging/vfreemem.c](paging/vfreemem.c)).

1. **`SYSCALL xmmap(int virtpage, bsd_t source, int npages)`**
   Much like its Unix counterpart (see `man mmap`), it maps a source file
   ("backing store" here) of size `npages` pages to the virtual page `virtpage`.
   A process may call this multiple times to map data structures, code, etc.
1. **`SYSCALL xmunmap(int virtpage)`**
   This call, like `munmap`, should remove a virtual memory mapping.
   See `man munmap` for the details of the Unix call.
1. **`SYSCALL vcreate(int *procaddr, int ssize, int hsize, int priority, char *name, int nargs, long args)`**
   This call will create a new Xinu process.
   The difference from `create()` (in [create.c](/sys/create.c)) is that the
   process' heap will be private and exist in its virtual memory.
   The size of the heap (in number of pages) is specified by the user through
   `hsize`.
   `create()` should be left (mostly) unmodified.
   Processes created with `create()` should not have a private heap, but should
   still be able to use `xmmap()`.
1. **`WORD *vgetmem(unsigned int nbytes)`**
   Much like `getmem()` (in [getmem.c](/sys/getmem.c)), `vgetmem()` will
   allocate the desired amount of memory if possible.
   The difference is that `vgetmem()` will get the memory from a process'
   private heap located in virtual memory.
   `getmem()` still allocates memory from the regular Xinu kernel heap.
1. **`SYSCALL srpolicy(int policy)`**
   This function will be used to set the page replacement policy.
   You will implement only one policy for this assignment, Second-Chance (SC),
   which will be the default policy.
   You can declare constant `SC` as `3`.
1. **`SYSCALL vfreemem(struct mblock *block, unsigned int size_in_bytes)`**
   You will implement a corresponding `vfreemem()` for `vgetmem()` call.
   `vfreemem()` takes two parameters and returns `OK` or `SYSERR`
    The two parameters are similar to those of the original `freemem()` in Xinu
    (see [freemem.c](/sys/freemem.c)).
    The type of the first parameter `block` depends on your own implementation.

## 3. Overall Organization

The following sections discuss at a high level the organization of the system,
the various pieces that need to be implemented in Xinu and how they relate to
each other.
You are welcome to use a different implementation strategy if you think it is
easier or better as long as it has the same functionality and challenges.

### 3.1 Memory and Backing Store

#### 3.1.1 Backing Stores

Virtual memory commonly uses disk spaces to extend the physical memory.
However, our version of Xinu has no file system support.
Instead, we will emulate the backing store (how it is emulated will be detailed
in [3.1.3](#313-backing-store-emulation)).
To access the backing store, you need to implement the following functions in
the directory [paging](/paging):

1. **`bsd_t`**
   is the type of backing store descriptors.
   Each descriptor is used to reference a backing store.
   Its type declaration is in [paging.h](/h/paging.h).
   This type is merely `unsigned int`.
   There are 8 backing stores.
   You will use the IDs 0 through 7 to identify them.
1. **`int get_bs(bsd_t bs_id, unsigned int npages)`**
   requests a new backing store with ID `bs_id` of size `npages` (in pages, not
   bytes).
   If a new backing store can be created, or a backing store with this ID
   already exists, the size of the new or existing backing store is returned.
   This size is in pages.
   If a size of 0 is requested, or the creation encounters an error, `SYSERR`
   should be returned.
    Also for practical reasons, `npages` should be no more than 256.
1. **`SYSCALL release_bs(bsd_t bs_id)`**
   releases the backing store with the ID `bs_id`.
1. **`SYSCALL read_bs(char *dst, bsd_t bs_id, int page)`**
   copies the `page`-th page from the backing store referenced by `bs_id` to
   `dst`.
   It returns `OK` on success, `SYSERR` otherwise.
   The first page of a backing store is page zero.
1. **`SYSCALL write_bs(char *src, bsd_t bs_id, int page)`**
   copies a page referenced by `src` to the `page`-th page of the backing store
   referenced by `store`.
   It returns `OK` on success, `SYSERR` otherwise.

#### 3.1.2 Memory Layout

The basic Xinu memory layout is as follows (page size = 4096 bytes):

```
--------------------------------------------
Virtual Memory       (pages 4096 and beyond)
--------------------------------------------
3072 Frames          (pages 1024 - 4095)
--------------------------------------------
Kernel Memory        (pages 406 - 1023)
--------------------------------------------
Kernel Memory        (pages 160 - 405)
--------------------------------------------
Kernel Memory        (pages 25 - 159)
--------------------------------------------
Xinu text, data, bss (pages 0 - 24)
--------------------------------------------
```

As you can see, our Xinu version compiles to about 100KB, or 25 pages.
There is an area of memory from page 160 through the end of page 405 that cannot
be used (this is referred to as the "HOLE" in
[initialize.c](/sys/initialize.c)).
We will place the free frames into pages 1024 through 4095, giving 3072 frames.

The frames will be used to store resident pages, page directories, and page
tables.
The remaining free memory below page 4096 is used for Xinu's kernel heap
(organized as a freelist).
`getmem()` and `getstk()` will obtain memory from this area (from the bottom and
top, respectively).

All memory below page 4096 will be global.
That is, it is usable and visible by all processes and accessible by simply
using actual physical addresses.
As a result, the first four page tables for every process will be the same, and
thus should be shared.

Memory at page 4096 and above constitute a process' virtual memory.
This address space is private and visible only to the process which owns it.
Note that the process' private heap is located somewhere in this area.
(Optionally, you can also place a private stack in this area, but this is not
required.)


#### 3.1.3 Backing Store Emulation

Since our version of Xinu does not have file system support, we need to emulate
the backing store with physical memory.
In particular, consider the following Xinu memory layout:

```
--------------------------------------------
Virtual Memory       (pages 4096 and beyond)
--------------------------------------------
8 Backing Stores     (pages 2048 - 4095)
--------------------------------------------
1024 Frames          (pages 1024 - 2047)
--------------------------------------------
Kernel Memory        (pages 406 - 1023)
--------------------------------------------
Kernel Memory        (pages 160 - 405)
--------------------------------------------
Kernel Memory        (pages 25 - 159)
--------------------------------------------
Xinu text, data, bss (pages 0 - 24)
--------------------------------------------
```

A Xinu instance has 16 MB (4096 pages) of real memory in total.
We reserve the top 8MB real memory as backing stores.
We have 8 backing stores and each backing store maps up to 256 pages (each page
is 4K size).
As a result, we have the following map between the backing store and the
corresponding physical memory range:

```
backing store 0: 0x00800000 - 0x008fffff
backing store 1: 0x00900000 - 0x009fffff
backing store 2: 0x00a00000 - 0x00afffff
backing store 3: 0x00b00000 - 0x00bfffff
backing store 4: 0x00c00000 - 0x00cfffff
backing store 5: 0x00d00000 - 0x00dfffff
backing store 6: 0x00e00000 - 0x00efffff
backing store 7: 0x00f00000 - 0x00ffffff
```

In the implementation, you need to "steal" physical memory frames 2048 - 4095
(take a close look at [sys/i386.c](sys/i386.c), and pay attention to the
variables `npages` and `maxaddr`).
As a result, this portion of memory will not be used for other purposes.
You can assume that our grading program will not modify this part of memory.

These 8 backing stores will serve a purpose similar to 
[swap space on disk](https://en.wikipedia.org/wiki/Memory_paging#Unix_and_Unix-like_systems) 
in a traditional operating system. Typically, you likely won't need to interact with
them directly when allocating memory, and will instead only access them when
evicting pages from virtual memory or loading evicted pages back. The only time
you will likely need to interact with them directly is when using `xmmap` and
`xmunmap`.

#### 3.1.4 Page Tables and Page Directories

Page tables and page directories (i.e. outer page tables) can be placed in any
free frames.
For this project you will not be paging either the page tables or page
directories.
As page tables are always resident in memory, it is not practical to allocate
all potential page tables for a process when it is created (you will, however,
allocate a page directory).
To map all 4 GB of memory would require 4 MB of page tables! To conserve memory,
page tables must be created on-demand.
That is, the first time a page is legally touched (i.e.
it has been mapped by the process) for which no page table is present, a page
table should be allocated.
Conversely, when a page table is no longer needed it should be removed to
conserve space.

### 3.2 Supporting Data Structures

#### 3.2.1 Finding the backing store for a virtual address

You may realize that there is a problem - if a process can map multiple address
ranges to different backing stores, how does one figure out which backing store
a page needs to be read from (or written to) when it is being brought into
(removed from) a frame?
To solve the problem, you need to keep track of which backing store is allocated
when a process is created by `vcreate()`.
Then, a particular page to write to/read from can be calculated using its
virtual page number within the related store.
You may need to declare a new kernel data structure which maps virtual address
spaces to backing store descriptors.
We will call this *the backing store map*.
It should be a tuple like:

```
{ pid, vpage, npages, store }
```

You should write a function that performs the lookup:

```
f(pid, vaddr) -> {store, pageoffset within store}
```

The function `xmmap()` will add a mapping to this table.
`xmunmap()` will remove a mapping from this table.

#### 3.2.2 Inverted Page Table

When writing out a dirty page you may notice the only way to figure out which
virtual page and process (and thus which backing store) a dirty frame belongs to
would be to traverse the page tables of every process looking for a frame
location that corresponds to the frame we wish to write out.
This is highly inefficient.
To prevent this, we use another kernel data structure, an *inverted page table*.
The inverted page table contains tuples like:

```
{ frame number, pid, virtual page number }
```

Of course, if we use an array of size `NFRAMES`, the frame number is implicit
and just the index into the array.
With this structure, we can easily find the pid and virtual page number of the
page held within any frame `i`.
From that we can easily find the backing store (using the backing store map) and
compute which page within the backing store corresponds with the page in frame
`i`.

You may also want to use this table to hold other information for page
replacement (i.e., any data needed to estimate page replacement policy).

### 3.3 Process Considerations

With each process having its own page directory and page tables, there are some
new considerations in dealing with processes.

#### 3.3.1 Process Creation

When a process is created we must also create a page directory and record its
address.
Also remember that the first 16 megabytes of each process will be mapped to the
16 megabytes of physical memory, so we must initialize the process' page
directory accordingly.
This is important as our backing stores also depend on this correct mapping.

A mapping must be created for the new process' private heap and stack, if
created with `vcreate()`.
As you are limited to 8 backing stores, you may want to use just one mapping for
both the heap and the stack (as with the kernel heap), `vgetmem()` taking from
one end and the stack growing from the other.
(Keeping a private stack and paging it is optional, but a private heap must be
maintained).

#### 3.3.2 Process Destruction

When a process dies, the following should happen:

* All frames which currently hold any of its pages should be written to the
  backing store and be freed.
* All of its mappings should be removed from the backing store map.
* The backing stores for its heap (and stack if have chosen to implement a
  private stack) should be released (remember backing stores allocated to a
  process should persist unless the process explicitly releases them).
* The frame used for the page directory should be released.

#### 3.3.3 Context Switching

It should also be clear that as we switch between processes we must also switch
between memory spaces.
This is accomplished by adjusting the PDBR register with every context switch.
We must be careful, however, as this register must always point to a valid page
directory which is in RAM at a page boundary.

Think carefully about where you place this switch if you put it in `resched()` -
before or after the actual context switch.

#### 3.3.4 System Initialization

The NULL process is somewhat of a special case, as it builds itself in the
function `sysinit()`.
The NULL process should not have a private heap (like any processes created with
`create()`).

The following should occur at system initialization:

* Set the DS and SS segments' limits to their highest values.
  This will allow processes to use memory up to the 4 GB limit without
  generating general protection faults.
  Make sure the initial stack pointer (`initsp`) is set to a real physical
  address (the highest physical address) as it is in normal Xinu.
  See `i386.c`.
  And don't forget to "steal" physical memory frames 2048 - 4096 for backing
  store purposes.
* Initialize all necessary data structures.
* Create the page tables which will map pages 0 through 4095 to the physical
  16 MB.
  These will be called the global page tables.
* Allocate and initialize a page directory for the NULL process.
* Set the PDBR register to the page directory for the NULL process.
* Install the page fault interrupt service routine.
* Enable paging.

### 3.4 The Interrupt Service Routine (ISR)

As you know, a page fault triggers an interrupt 14.
When an interrupt occurs the machine pushes `CS:IP` and then an error code (see
Intel Volume III chapter 5):

```
----------
error code
----------
    IP
----------
    CS
----------
    ...
    ...
```

It then jumps to a predetermined point, the ISR.
To specify the ISR we use the following routine (see [evec.c](/sys/evec.c)):

```c
set_evec(int interrupt, (void (*isr)(void))) 
```

### 3.5 Faults and Replacement Policy

#### 3.5.1 Page Faults

A page fault indicates one of two things: the virtual page on which the faulted
address exists is not present or the page table which contains the entry for the
page on which the faulted address exists is not present.
To deal with a page fault you must do the following:

* Get the faulted address `a`.
* Let `vp` be the virtual page number of the page containing the faulted
  address.
* Let `pd` point to the current page directory.
* Check that `a` is a legal address (i.e. that it has been mapped in `pd`).
  If it is not, print an error message and kill the process.
* Let `p` be the upper ten bits of `a`. [What does `p` represent?]
* Let `q` be the bits [21:12] of `a`. [What does `q` represent?]
* Let `pt` point to the `p`-th page table.
  If the `p`-th page table does not exist, obtain a frame for it and initialize
  it.
* To page in the faulted page do the following:
  * Using the backing store map, find the store `s` and page offset `o` which
    correspond to `vp`.
  * In the inverted page table, increase the reference count of the frame that
    holds `pt`.
    This indicates that one more of `pt`'s entries is marked as "present."
  * Obtain a free frame, `f`.
  * Copy the page `o` of store `s` to `f`.
  * Update `pt` to mark the appropriate entry as present and set any other
    fields.
    Also set the address portion within the entry to point to frame `f`.

#### 3.5.2 Obtaining a Free Frame

When a free frame is needed, it may be necessary to remove a resident page from
frame.
How you pick the page to remove depends on your page replacement policy.

Your function to find a free page should do the following:

* Search inverted page table for an empty frame.
  If one exists, stop.
* Else, pick a page to replace.
* Using the inverted page table, get `vp`, the virtual page number of the page
  to be replaced.
* Let `a` be `vp*4096` (the first virtual address on page `vp`).
* Let `p` be the high 10 bits of `a`.
  Let `q` be bits [21:12] of `a`.
* Let `pid` be the pid of the process owning `vp`.
* Let `pd` point to the page directory of process `pid`.
* Let `pt` point to the `pid`'s `p`-th page table.
* Mark the appropriate entry of `pt` as not present.
* If the page being removed belongs to the current process, invalidate the TLB
  entry for the page `vp` using the `invlpg` instruction (see Intel Volume
  III/II).
* In the inverted page table, decrement the reference count of the frame
  occupied by `pt`.
  If the reference count has reached zero, you should mark the appropriate entry
  in `pd` as being not present.
  This conserves frames by keeping only page tables which are necessary.
* If the dirty bit for page `vp` was set in its page table you must do the
  following:
  * Use the backing store map to find the store and page offset within store
    given `pid` and `a`.
    If the lookup fails, something is wrong.
    Print an error message and kill the process pid.
  * Write the page back to the backing store.

#### 3.5.3 Page Replacement Policy - Second-Chance (SC)

You must implement the following replacement algorithm called Second-Chance
(`SC`).
When a frame is allocated for a page, you insert the frame into a circular
queue.
When a page replacement occurs, `SC` first looks at the current position in the
queue (current position starts from the head of the queue) and checks to see
whether its reference bit is set (i.e., `pt_acc = 1`).
If it is not set, the page is swapped out.
Otherwise, the reference bit is cleared, the current position moves to the next
page and this process is repeated.
If all the pages have their reference bits set, on the second encounter, the
page will be swapped out, as it now has its reference bit cleared.

When `srpolicy(SC)` is called in `main()`, your program should turn on a
debugging option, so that when replacements occur, it will print ONLY the
replaced frame numbers (not addresses) for grading.

Note that you are free to add whatever structures you'd like in your inverted
page table.

## 4. Required API Calls

You must implement the system calls listed at the beginning of this handout
exactly as specified.
Be sure to check the parameters for validity.
For example, no process should be allowed to remap the lowest 16 MB of the
system (global memory).

Also, none of Xinu's other system call interfaces should be modified.

## 5. Details on the Intel Architecture and Xinu

After having read chapters two and three in
[Volume 3][intel_system_programming_model] you should have a basic understanding
of the details of memory management in the Intel architecture.

The following might be useful for you to know:

* We are using the Intel Pentium chip, not the Pentium Pro or Pentium II.
  Some details of those chips do not apply.
* Xinu uses the flat memory model, i.e. physical address = linear addresses.
* The segments are set in [i386.c](/sys/i386.c) in the function `setsegs()`.
* Pages are 4k (4096 bytes) in size.
  Do not use 2M or 4M page size.
* The backend machines have 16 MB (4096 pages) of real memory.
* Some example code is given in the project directory for getting and setting
  the control registers.
  A useful function, `dump32(unsigned long)`, for dumping a binary number with
  labeled bits is also given (in [paging/dump32.c](/paging/dump32.c)).

## 6. Test File and Sample Output

### 6.1 `testmain.c`

```c
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

#define PROC1_VADDR 0x40000000
#define PROC1_VPNO  0x40000
#define PROC2_VADDR 0x80000000
#define PROC2_VPNO  0x80000
#define TEST1_BS    1

void proc1_test1(char *msg, int lck) {
  char *addr;
  int i;

  get_bs(TEST1_BS, 100);

  if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
    kprintf("xmmap call failed\n");
    sleep(3);
    return;
  }

  addr = (char *) PROC1_VADDR;
  for (i = 0; i < 26; ++i) {
    *(addr + (i * NBPG)) = 'A' + i;
  }

  sleep(6);

  for (i = 0; i < 26; ++i) {
    kprintf("0x%08x: %c\n", addr + (i * NBPG), *(addr + (i * NBPG)));
  }

  xmunmap(PROC1_VPNO);
  return;
}

void proc1_test2(char *msg, int lck) {
  int *x;

  kprintf("ready to allocate heap space\n");
  x = vgetmem(1024);
  kprintf("heap allocated at %x\n", x);
  *x = 100;
  *(x + 1) = 200;

  kprintf("heap variable: %d %d\n", *x, *(x + 1));
  vfreemem(x, 1024);
}

void proc1_test3(char *msg, int lck) {

  char *addr;
  int i;

  addr = (char*) 0x0;

  for (i = 0; i < 1024; ++i) {
    *(addr + (i * NBPG)) = 'B';
  }

  for (i = 0; i < 1024; ++i) {
    kprintf("0x%08x: %c\n", addr + (i * NBPG), *(addr + (i * NBPG)));
  }

  return;
}

int main() {
  int pid1;
  int pid2;

  kprintf("\n1: shared memory\n");
  pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 0, NULL);
  resume(pid1);
  sleep(10);

  kprintf("\n2: vgetmem/vfreemem\n");
  pid1 = vcreate(proc1_test2, 2000, 100, 20, "proc1_test2", 0, NULL);
  kprintf("pid %d has private heap\n", pid1);
  resume(pid1);
  sleep(3);

  kprintf("\n3: Frame test\n");
  pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
  resume(pid1);
  sleep(3);
}
```

### 6.2 Sample output

```
testmain.c sample output:
1: shared memory
0x40000000: A
0x40001000: B
0x40002000: C
0x40003000: D
0x40004000: E
0x40005000: F
0x40006000: G
0x40007000: H
0x40008000: I
0x40009000: J
0x4000a000: K
0x4000b000: L
0x4000c000: M
0x4000d000: N
0x4000e000: O
0x4000f000: P
0x40010000: Q
0x40011000: R
0x40012000: S
0x40013000: T
0x40014000: U
0x40015000: V
0x40016000: W
0x40017000: X
0x40018000: Y
0x40019000: Z

2: vgetmem/vfreemem
pid 47 has private heap
ready to allocate heap space
heap allocated at 1000000
heap variable: 100 200

3: Frame test
0x00000000: B
0x00001000: B
0x00002000: B
0x00003000: B
0x00004000: B
0x00005000: B
0x00006000: B
0x00007000: B
0x00008000: B
```

## 7. Debugging

Please try to debug by yourself first.
Also realize that you know your program best.

Furthermore, if it helps you, you can uncomment the `#define`'s in
[evec.c](/sys/evec.c) to get a stack trace and register dump.
Using this and `nm` on the file `xinu.elf` can help you locate where your
program crashed.
Or you may recompile everything using the compiler's `-g` flag, disassemble
`xinu.elf` using `objdump -d xinu.elf > xinu.dis`, load `xinu.dis` into your
text editor and search for the return address in the stack.
In the disassembly the addresses are the numbers on the left (e.g. `ab3e:`).
This will show you the function name (may be some lines above) of the function
the crash occurred in and (if you compiled that particular file with `-g`) the C
line number in the []'s.

The most difficult problem to diagnose is when the machine simply reboots
itself.
This is usually the result of having a bad stack pointer.
In such a case the machine cannot give a trap.

## 8. What to Turn In

The goal of this assignment is to provide support for:

* **Memory mapping**: mapping of the first 16 Mb of physical memory, and the
  `xmmap()` and `xmunmap()` system calls
  * Different running processes created with `vcreate` can have its own private
    heap.
  * `vgetmem`, `vfreemem`: implemented and fully functional.
  * All running processes can simply share the same page table.
* **Demand paging**: data is retrieved from the backing stores only when needed.
* **Backing store management**:
  * `get_bs`, `release_bs`, `read_bs`, `write_bs`: implemented and fully
    functional
* **Page replacement policy**: `SC`

Remember that, per the specification, page tables are created and destructed on
demand.
In other words, your system must not pre-allocate page tables.
Also, page tables that do not contain at least one valid entry pointing to a
data page should be destroyed (the frame should be freed) as soon as their last
entry is invalidated.
Page tables and page directories are not paged out.

### 8.1 Turn-in Instructions

1. Preparation
   * You can write code in `main.c` to test your procedures, but please note
     that when we test your programs we will replace the `main.c` file!
     Therefore, do not put any functionality in the `main.c` file.
   * Also, ALL debugging output MUST be turned off before you submit your code.
2. Submission
   * Go to the [compile](compile/) directory and do `make clean`.
   * Add new files to your repository using `git add <filepath>` command.
   * Create a directory `TMP_unityid`, where `unityid` is your Unity ID (e.g.,
     `TMP_xinurocks`), under the root of the project, and copy all the files you
     have modified/created, both `.c` files and `.h` files, into the new
     directory.
     Then, add the directory to the repository by doing `git add TMP_xinurocks`
     (which will add all the files under the directory).
   * Check if all the necessary changes have been staged by using `git status`.
   * Then, commit and push:
     ```shell
     git commit -am 'Done'
     git push
     ```
     You can do commit and push whenever you want, but the final submission must
     be committed/pushed with the message `Done`.
     For a final check, go to your repository using a web browser, and check if
     your changes have been pushed.

## 9. One Last Note

Even with the design given to you this is not necessarily an easy project.
Dealing with low level aspects of the machine is always difficult.
Please do not procrastinate.
It is very easy (especially with Xinu and even more so when working at a low
level) to run into problems.

## 10. Grading Policy

* (65 pts) Submission can be compiled and pass the provided `testmain.c`
  successfully
* (35 pts) Seven additional test cases (5 pts/testcase)

## FAQs

**How do I get the virtual page number from a virtual address?**

* The most significant 20 bits of a virtual address form the virtual page
  number.

**About the mapping `< pid, vpage, npages, store >`**

* This mapping is maintained inside the kernel.
  Since the "store"  can take only 8 values (because there are only 8 backing
  stores possible for any user), and no store can be mapped to more than one
  range of virtual memory at any time, the table that contains these mappings
  will contain only 8 entries.
  This table is placed in kernel data segment in the first 25 pages of the
  physical memory
  You need not take any extra care about placing this table.
  Just create an array of 8 entries of the type of the mapping and that's all.
  It is pretty similar to `semaph[]` and `proctab[]`.

* Note that the above only applies to backing stores that are in use by a section
  of virtual memory being used as a virtual heap. The same backing store can be
  mapped to multiple regions of virtual memory if `xmmap` is used as a means of
  allowing the processes to share disk space and communicate data as in the provided
  tests.

**`Paging.h` contains two structures `pd_t` and `pt_t` which contains a lot of bit fields. Initially which fields should be set in a page directory and a page table?**

* For page directories, set the following bits and make the other bits zero:
  `pd_write` always and `pd_pres` whenever the corresponding page tables are
  present in the main memory.
* For the four global page tables, set the following bits: `pt_pres` and
  `pt_write`.
  You can make others zero.
* (This answer should be fairly obvious if you have read the Intel manuals
  carefully.)

**Where do we place the page tables and page directories?**

* If your memory is divided into 4096 pages, page tables and page directories
  should be placed in the range 1024-2047 (was 1024-1535, but it should be 2047
  in our configuration).
  They should be placed on page boundaries only, i.e., the starting address of
  any page table or page directory should be divisible by the size of the pages
  `NBPG`.

**What is the use of `xmmap()`?**

* There was a big misconception about the usage of `xmmap()` among many previous
  students.
  When does a user process call `xmmap()`? Why is it used for?
* Even though `xmmap()` is given in the first page of your handout, it is not
  the most important system call that you should implement.
  Also, it is not main part of the project.
  Also, it is not the only way by which you can access virtual memory and test
  your implementation.
* Then, how else can a process try to use virtual memory? Here is one example.
  This example shows how to access virtual memory, and when a page fault could
  happen.
  ```c
  main() {
    /* Process A is created with a virtual heap of 100 pages. */
    vcreate(process A, , , hsize = 100, ,,,);
  }

  process A() {
    int *x;
    int temp;
    /* Allocates some memory in the virtual heap which is in virtual memory. */
    x = vgetmem(1000);
    /**
     * The following  statement will cause a page fault. The page fault handling
     * routing will read in the required page from backing store into the main
     * memory, set the proper page tables and the page directory entries and
     * reexecute the statement.
     */
    *x = 100;
    x++;
    *x = 200;
    /**
     * You are reading back from the virtual heap to check if the previous write
     * was successful.
     */
    temp = *x;

    vfreemem(--x, 1000); /* Frees the allocation in the virtual heap. */
  }
  ```
* The virtual heap in the example is present in a backing store that is
  exclusive for the process alone (no backing store should be shared across
  processes; neither should the same backing store be mapped to multiple memory
  ranges).
  The heap is mapped in the address range of 4096th page to 4196th page of the
  process.
  So, the backing store mapping table should contain an entry
  `< process A's pid, 4096, 100, backing store number >`.
* Then, why do we need `xmmap()` and what does it do? `xmmap()` is very similar
  to `mmap()` of Unix.
  It treats the backing stores as "files." One potential usage of `xmmap()` is
  as follows:
  ```c
  Process A:
    char *x;
    char temp;
    get_bs(4, 100);
    xmmap(7000, 4, 100); /* This call simply creates an entry in the backing store mapping. */
    x = 7000*4096;
    *x = 'Y'; /* Write into virtual memory, will create a fault and system should proceed as in the prev example. */
    temp = *x; /* Read back and check. */
    xmunmap(...);
  ```
* It can be thought of as you create a file, whose name is "4".
  It is a big empty file of size 100 pages.
  You store a character 'A' as the first character in the file.
  However, instead of using file I/O operations, you modify the file by means of
  a memory modification, as you have mapped the file to a memory location.
* Let us say there is another process B which executes the following code after
  process A writes to its virtual memory mapped by `xmmap`.
  ```c
  Process B:
    char *x;
    char temp_b;
    xmmap(6000, 4, 100);
    x = 6000 * 4096;
    temp_b = *x: /* Surprise: Now, temp_b will get the value 'Y' written by the process A to this backing store '4'. */
  ```
* These examples should make the usage of `xmmap()` more clear.
  Think about it.

**Page fault handling routine (page fault ISR) - What should be done here?**

* Psuedo code for the implementation (which would be easier if you do it in
  assembly):
  ```
  1) store error code in a global variable
  2) store flag register
  3) clear all interrupts
  4) save all general registers
  5) page fault handling
  6) restore general registers
  7) restore flag register
  8) iret
  ```
  If you have not written in assembly language before, look at some code written
  in assembly in Xinu itself.
  Or else, disassemble some simple C code and check the assembly code.
  Note that not everything has to be implemented in assembly as it would be very
  difficult.
  Thus, you could include a call to a C function which handles 5).

**Are `read_bs` and `write_bs` blocking calls and can they be used inside our interrupt handling routine?**

* They are, and can be used inside the page fault handling routine.

**How do I test replacement policies?**

* There is a constant `NFRAMES` in [paging.h](/h/paging.h).
  Its default value is 1024.
  Make sure that your entire code depends on this constant as a measure of the
  available free frames.
  If we change the constant value to say 400, then the number of free frames
  initially available is only 400, i.e., your main memory looks as if it has
  only 1024 + `NFRAMES` = 1024 + 400 = 1424 frames of memory.
  So, you have an ample scope to test your replacement policy by changing
  `NFRAMES`.

[address_translation_example]: https://mankiyoon.github.io/courses/csc501/files/intelvm.html
[intel_system_programming_model]: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html

**What is the hole for**

* The hole is a section of memory used for certain low-level tasks such as boot loading and monitors. You will not
  need to worry about interacting with it for this assignment. Just ensure that it is mapped correctly and that
  you do not erroneously write into it as that may cause issues.
