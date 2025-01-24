/* screate.c - screate, newsem */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lab0.h>
LOCAL int newsem();

/*------------------------------------------------------------------------
 * screate  --  create and initialize a semaphore, returning its id
 *------------------------------------------------------------------------
 */
SYSCALL screate(int count)
{
	unsigned long start_time = 0;
    if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SCREATE].frequency++;  // Increment frequency for freemem
    }

	STATWORD ps;    
	int	sem;

	disable(ps);
	if ( count<0 || (sem=newsem())==SYSERR ) {
		restore(ps);
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SCREATE].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
	semaph[sem].semcnt = count;
	/* sqhead and sqtail were initialized at system startup */
	restore(ps);
	if (syscall_tracing) {
        unsigned long end_time = ctr1000;
        syscall_table[currpid][SYSCALL_SCREATE].time += (end_time - start_time); // Add time spent
    }
	return(sem);
}

/*------------------------------------------------------------------------
 * newsem  --  allocate an unused semaphore and return its index
 *------------------------------------------------------------------------
 */
LOCAL int newsem()
{
	int	sem;
	int	i;

	for (i=0 ; i<NSEM ; i++) {
		sem=nextsem--;
		if (nextsem < 0)
			nextsem = NSEM-1;
		if (semaph[sem].sstate==SFREE) {
			semaph[sem].sstate = SUSED;
			return(sem);
		}
	}
	return(SYSERR);
}
