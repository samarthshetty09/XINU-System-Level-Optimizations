/* setdev.c - setdev */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lab0.h>
/*------------------------------------------------------------------------
 *  setdev  -  set the two device entries in the process table entry
 *------------------------------------------------------------------------
 */
SYSCALL	setdev(int pid, int dev1, int dev2)
{	
	unsigned long start_time = 0;
    if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_SETDEV].frequency++;  // Increment frequency for freemem
    }
	short	*nxtdev;

	if (isbadpid(pid)) {
		if (syscall_tracing) {
			unsigned long end_time = ctr1000;
			syscall_table[currpid][SYSCALL_SETDEV].time += (end_time - start_time); // Add time spent
		}
		return(SYSERR);
	}
		
	nxtdev = (short *) proctab[pid].pdevs;
	*nxtdev++ = dev1;
	*nxtdev = dev2;

    if (syscall_tracing) {
		unsigned long end_time = ctr1000;
		syscall_table[currpid][SYSCALL_SETDEV].time += (end_time - start_time); // Add time spent
	}
	return(OK);
}
