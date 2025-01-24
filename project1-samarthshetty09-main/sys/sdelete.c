/* sdelete.c - sdelete */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lab0.h>
/*------------------------------------------------------------------------
 * sdelete  --  delete a semaphore by releasing its table entry
 *------------------------------------------------------------------------
 */
SYSCALL sdelete(int sem)
{
	unsigned long start_time = 0;
    if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SDELETE].frequency++;  // Increment frequency for freemem
    }

	STATWORD ps;    
	int	pid;
	struct	sentry	*sptr;

	disable(ps);
	if (isbadsem(sem) || semaph[sem].sstate==SFREE) {
		restore(ps);
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SDELETE].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
	sptr = &semaph[sem];
	sptr->sstate = SFREE;
	if (nonempty(sptr->sqhead)) {
		while( (pid=getfirst(sptr->sqhead)) != EMPTY)
		  {
		    proctab[pid].pwaitret = DELETED;
		    ready(pid,RESCHNO);
		  }
		resched();
	}
	restore(ps);
	if (syscall_tracing) {
        unsigned long end_time = ctr1000;
        syscall_table[currpid][SYSCALL_SDELETE].time += (end_time - start_time); // Add time spent
    }
	
	

	return(OK);
}
