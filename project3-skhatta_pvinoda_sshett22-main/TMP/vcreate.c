/* vcreate.c - vcreate */

#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr, ssize, hsize, priority, name, nargs, args)
int *procaddr; /* procedure address		*/
int ssize;     /* stack size in words		*/
int hsize;     /* virtual heap size in pages	*/
int priority;  /* process priority > 0		*/
char *name;    /* name (for debugging)		*/
int nargs;     /* number of args that follow	*/
long args;     /* arguments (treated like an	*/
               /* array in the code)		*/
{
    STATWORD ps;
    disable(ps);

    int pid;   // Process ID
    int bs_id; // Backing store ID
    int vpno;  // Starting virtual page number
    struct mblock *bs_badd;

    // Validate hsize
    if (hsize <= 0 || hsize > 256)
    {
        restore(ps);
        return SYSERR;
    }

    // Create the new process
    pid = create(procaddr, ssize, priority, name, nargs, args);
    if (pid == SYSERR)
    {
        release_bs(bs_id); // release the backing store
        restore(ps);
        return SYSERR;
    }

    // get a free backing store id to allocate the private heap
    if (get_bsm(&bs_id) == SYSERR)
    {
        restore(ps);
        return SYSERR;
    }

    vpno = VHEAP_START;
    // Map the allocated backing store to the process
    bsm_map(pid, vpno, bs_id, hsize);

    bsm_tab[bs_id].bs_private = 1; // Set flag to indicate private heap

    // Set up the process table for vcreate specifics
    proctab[pid].store = bs_id;     // assign backing store id
    proctab[pid].vhpno = vpno;      // starting vpno
    proctab[pid].vhpnpages = hsize; // total pages in virtual heap

    // initializing the heap in the backing store
    bs_badd = (BACKING_STORE_BASE + (bs_id * BACKING_STORE_UNIT_SIZE));
    bs_badd->mnext = NULL;
    bs_badd->mlen = hsize * NBPG; // set heap size in bytes

    // initialize the process's vmemlist (heap management)
    proctab[pid].vmemlist->mnext = (vpno * NBPG); // Starting virtual address

    restore(ps); // restore interrupts
    return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL newpid()
{
    int pid; /* process id to return		*/
    int i;

    for (i = 0; i < NPROC; i++)
    { /* check all NPROC slots	*/
        if ((pid = nextproc--) <= 0)
            nextproc = NPROC - 1;
        if (proctab[pid].pstate == PRFREE)
            return (pid);
    }
    return (SYSERR);
}