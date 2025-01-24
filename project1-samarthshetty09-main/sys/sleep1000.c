/* sleep1000.c - sleep1000 */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sleep.h>
#include <stdio.h>
#include <lab0.h>
/*------------------------------------------------------------------------
 * sleep1000 --  delay the caller for a time specified in 1/100 of seconds
 *------------------------------------------------------------------------
 */
SYSCALL sleep1000(int n)
{
	unsigned long start_time = 0;
	if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SLEEP1000].frequency++;  // Increment frequency for freemem
    }
	STATWORD ps;    

	if (n < 0  || clkruns==0) {
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SLEEP1000].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
	disable(ps);
	if (n == 0) {		/* sleep1000(0) -> end time slice */
	        ;
	} else {
		insertd(currpid,clockq,n);
		slnempty = TRUE;
		sltop = &q[q[clockq].qnext].qkey;
		proctab[currpid].pstate = PRSLEEP;
	}
	resched();
        restore(ps);
	if (syscall_tracing) {
        unsigned long end_time = ctr1000;
        syscall_table[currpid][SYSCALL_SLEEP1000].time += (end_time - start_time); // Add time spent
    }
	return(OK);
}
