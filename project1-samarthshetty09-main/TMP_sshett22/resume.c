/* resume.c - resume */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

/*------------------------------------------------------------------------
 * resume  --  unsuspend a process, making it ready; return the priority
 *------------------------------------------------------------------------
 */
SYSCALL resume(int pid)
{
	unsigned long start_time = 0;
    if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_RESUME].frequency++;  // Increment frequency for freemem
    }
	STATWORD ps;    
	struct	pentry	*pptr;		/* pointer to proc. tab. entry	*/
	int	prio;			/* priority to return		*/

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate!=PRSUSP) {
		restore(ps);
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_RESUME].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
	prio = pptr->pprio;
	ready(pid, RESCHYES);
	restore(ps);
	if (syscall_tracing) {
        unsigned long end_time = ctr1000;
        syscall_table[currpid][SYSCALL_RESUME].time += (end_time - start_time); // Add time spent
    }
	return(prio);
}
