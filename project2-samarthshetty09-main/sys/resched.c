/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sched.h>
#include <math.h>

unsigned long currSP; /* REAL sp of current process */
extern int ctxsw(int, int, int, int);

/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */

int resched()
{
    register struct pentry *optr; /* Pointer to the old (current) process */
    register struct pentry *nptr; /* Pointer to the new process */

    int proc = 0;
    int max = 0;

    int nxt_proc = firstid(rdyhead);

    /*Exponential Scheduler*/
    if (getschedclass() == EXPDISTSCHED) {
        int r_v = (int)expdev(0.1);      // random value
        optr = &proctab[currpid];        // get pointer to current process

        // only null process is in the queue
        if (nxt_proc == NULLPROC && q[rdyhead].qnext == q[rdytail].qprev) {
            if (optr->pstate != PRCURR) {
                nptr = &proctab[(currpid = dequeue(NULLPROC))]; // deque the null process and switch
                nptr->pstate = PRCURR;
#ifdef RTCLOCK
                preempt = QUANTUM; // reset quantum for the new current process
#endif
                ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                return OK;
            } else {
#ifdef RTCLOCK
                preempt = QUANTUM; // maintain current quantum for running process (NULLPROC)
#endif
                return OK;
            }
        }

        // If there are other processes in the ready queue
        if (nonempty(rdytail)) {
            // Traverse the queue to find a process with priority less than or equal to the generated random value
            while (nonempty(nxt_proc) && priori(nxt_proc) < r_v) {
                nxt_proc = q[nxt_proc].qnext;
            }
            // Compare priority of the current process and the selected process
            // If current process is still running and any one of the below case satisfies then dont switch
            // case1: random_val < cur_running_process < selected_process
            // case2: random_val <= selected_process < cur_running_process
            // case3: selected_process < cur_running_process < random_val
            if (optr->pstate == PRCURR) {
                if (
                    ((optr->pprio > r_v && (priori(nxt_proc) > optr->pprio || priori(nxt_proc) <= r_v)) ||
                     (optr->pprio < r_v && priori(nxt_proc) < optr->pprio))) {
#ifdef RTCLOCK
                    preempt = QUANTUM; // Maintain the current quantum for running process
#endif
                    return OK;
                } else {
                    // Change state of current process if it's still running
                    optr->pstate = PRREADY;
                    insert(currpid, rdyhead, optr->pprio); // Insert the current process back into the ready queue
                }
            }

            // Switch to the next ready process
            nptr = &proctab[(currpid = dequeue(nxt_proc))];
            nptr->pstate = PRCURR;
#ifdef RTCLOCK
            preempt = QUANTUM; // Reset the quantum for the new process
#endif
            ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
            return OK;
        }

#ifdef RTCLOCK
        preempt = QUANTUM; // Reset quantum as a failsafe
#endif
        return OK;
    }

/* Linux Like Scheduler   */  
    else if (getschedclass() == LINUXSCHED) {
        optr = &proctab[currpid];
        int max_goodness = -1, next_pid;

        // calculate dynamically the goodness for current process
        if (optr->pstate == PRCURR ) {
            optr->counter = preempt; // Remaining time quantum (extern var)
            if (optr->counter <= 0 || currpid == NULLPROC) {
                optr->goodness = 0;
                optr->counter = 0;
            } else {
                optr->goodness = optr->counter + optr->pprio;
            }
        }

        int need_new_epoch = 1, pid;
        next_pid = NULLPROC;    // set default process to null process
        // iterate over the ready q to find max goodness also check if a new epoch is needed
        for (pid = q[rdyhead].qnext; pid != rdytail; pid = q[pid].qnext) {
            if (proctab[pid].goodness > 0) {
                need_new_epoch = 0;
                if (proctab[pid].goodness > max_goodness) {
                    max_goodness = proctab[pid].goodness;
                    next_pid = pid;
                }
            }
        }

        // also check for the current process if its not in the ready queue
        if (optr->pstate == PRCURR && optr->goodness > 0) {
            need_new_epoch = 0;
            if (optr->goodness > max_goodness) {
                max_goodness = optr->goodness;
                next_pid = currpid;
            }
        }

        // check epoch falg
        if (need_new_epoch) {
            start_new_epoch();
        }

        // if no runnable processes have goodness > 0, run the NULL process
        if (max_goodness <= 0) {
            next_pid = NULLPROC;
        }

        nptr = &proctab[next_pid];

        // If the current process continues to run we reset its preemption counter
        if (currpid == next_pid) {
            preempt = optr->counter;
            return;
        } else {
            // else do a context switch to the new process
            if (optr->pstate == PRCURR) {
                optr->pstate = PRREADY;
                insert(currpid, rdyhead, optr->pprio);
            }

            nptr->pstate = PRCURR;
            dequeue(next_pid);
            currpid = next_pid;
            preempt = nptr->counter;

#ifdef RTCLOCK
            preempt = nptr->counter;
#endif
            ctxsw((int)&optr->pesp, (int)optr->pirmask,
                (int)&nptr->pesp, (int)nptr->pirmask);

            return;
        }
    }

    /* Default priority-based scheduler */
    else
    {
        register struct pentry *optr;
        register struct pentry *nptr;

        /* No switch needed if current process priority higher than next */
        if (((optr = &proctab[currpid])->pstate == PRCURR) &&
            (lastkey(rdytail) < optr->pprio))
        {
            return (OK);
        }

        /* Force context switch */

        if (optr->pstate == PRCURR)
        {
            optr->pstate = PRREADY;
            insert(currpid, rdyhead, optr->pprio);
        }

        /* Remove highest priority process at end of ready list */

        nptr = &proctab[(currpid = getlast(rdytail))];
        nptr->pstate = PRCURR; /* mark it currently running	*/
#ifdef RTCLOCK
        preempt = QUANTUM; /* reset preemption counter	*/
#endif

        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
        return OK;
    }
}
