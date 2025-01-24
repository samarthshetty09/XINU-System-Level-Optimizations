/* getpid.c - getpid */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lab0.h>
/*------------------------------------------------------------------------
 * getpid  --  get the process id of currently executing process
 *------------------------------------------------------------------------
 */
SYSCALL getpid()
{	
	unsigned long start_time = 0;
	if (syscall_tracing) {
        start_time = ctr1000;
        syscall_table[currpid][SYSCALL_GETPID].frequency++;  // Increment frequency for freemem
    }
	if (syscall_tracing) {
        unsigned long end_time = ctr1000;
        syscall_table[currpid][SYSCALL_GETPID].time += (end_time - start_time); // Add time spent
    }
	return(currpid);
}
