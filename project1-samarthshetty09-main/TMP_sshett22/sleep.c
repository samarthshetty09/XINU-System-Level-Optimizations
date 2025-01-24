/* sleep.c - sleep */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sleep.h>
#include <stdio.h>
#include <lab0.h>


/*------------------------------------------------------------------------
 * sleep  --  delay the calling process n seconds
 *------------------------------------------------------------------------
 */
SYSCALL	sleep(int n)
{	
	unsigned long start_time = 0;
    if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SLEEP].frequency++;  // Increment frequency for freemem
    }
	STATWORD ps;    
	if (n<0 || clkruns==0) {
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SLEEP].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
		
	if (n == 0) {
	        disable(ps);
		resched();
		restore(ps);
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SLEEP].time += (end_time - start_time); // Add time spent
		}
		return(OK);
	}
	while (n >= 1000) {
		sleep10(10000);
		n -= 1000;
	}
	if (n > 0)
		sleep10(10*n);
	if (syscall_tracing) {
		unsigned long end_time = ctr1000;
		syscall_table[currpid][SYSCALL_SLEEP].time += (end_time - start_time); // Add time spent
	}
	return(OK);
}
