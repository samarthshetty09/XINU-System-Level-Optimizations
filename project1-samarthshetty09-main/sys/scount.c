/* scount.c - scount */

#include <conf.h>
#include <kernel.h>
#include <sem.h>
#include <proc.h>
#include <lab0.h>
/*------------------------------------------------------------------------
 *  scount  --  return a semaphore count
 *------------------------------------------------------------------------
 */
SYSCALL scount(int sem)
{
extern	struct	sentry	semaph[];
	unsigned long start_time = 0;
    if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SCOUNT].frequency++;  // Increment frequency for freemem
    }

	if (isbadsem(sem) || semaph[sem].sstate==SFREE) {
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SCOUNT].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
	if (syscall_tracing) {
		unsigned long end_time = ctr1000;
		syscall_table[currpid][SYSCALL_SCOUNT].time += (end_time - start_time); // Add time spent
	}
	return(semaph[sem].semcnt);
}
