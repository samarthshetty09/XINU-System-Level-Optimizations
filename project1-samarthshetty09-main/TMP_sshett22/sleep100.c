/* sleep100.c - sleep100 */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sleep.h>
#include <stdio.h>
#include <lab0.h>

/*------------------------------------------------------------------------
 * sleep100  --  delay the caller for a time specified in 1/100 of seconds
 *------------------------------------------------------------------------
 */
SYSCALL sleep100(int n)
{
	STATWORD ps;
	unsigned long start_time;    
	if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SLEEP100].frequency++;  // Increment frequency for freemem
    }
	if (n < 0  || clkruns==0) {
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SLEEP100].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
	         
	disable(ps);
	if (n == 0) {		/* sleep100(0) -> end time slice */
	        ;
	} else {
		insertd(currpid,clockq,n*10);
		slnempty = TRUE;
		sltop = &q[q[clockq].qnext].qkey;
		proctab[currpid].pstate = PRSLEEP;
	}
	resched();
        restore(ps);
	if (syscall_tracing) {
		unsigned long end_time = ctr1000;
		syscall_table[currpid][SYSCALL_SLEEP100].time += (end_time - start_time); // Add time spent
	}
	return(OK);
}
