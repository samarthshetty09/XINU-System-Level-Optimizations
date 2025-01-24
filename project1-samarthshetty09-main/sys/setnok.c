/* setnok.c - setnok */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>
/*------------------------------------------------------------------------
 *  setnok  -  set next-of-kin (notified at death) for a given process
 *------------------------------------------------------------------------
 */
SYSCALL	setnok(int nok, int pid)
{	
	unsigned long start_time = 0;
    if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SETNOK].frequency++;  // Increment frequency for freemem
    }
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid)) {
		restore(ps);
		if (syscall_tracing) {
        	unsigned long end_time = ctr1000;
        	syscall_table[currpid][SYSCALL_SETNOK].time += (end_time - start_time); // Add time spent
    	}
		return(SYSERR);
	}
	pptr = &proctab[pid];
	pptr->pnxtkin = nok;
	restore(ps);
	if (syscall_tracing) {
        unsigned long end_time = ctr1000;
        syscall_table[currpid][SYSCALL_SETNOK].time += (end_time - start_time); // Add time spent
    }
	return(OK);
}
