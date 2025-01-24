#include <stdio.h>
#include "../h/sched.h"
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

int currClassSchd;

void setschedclass (int sched_class) {
    currClassSchd = sched_class;
}

int getschedclass() {
    return currClassSchd;
}

void start_new_epoch() {
    int i;
    struct pentry *proc;

    for (i = 0; i < NPROC; i++) {
        proc = &proctab[i];

        if (proc->pstate != PRFREE ) {
            // Calculate new quantum
            if (proc->counter > 0 && proc->counter != proc->quantum) {
                proc->quantum = (proc->counter / 2) + proc->pprio;
            } else {
                proc->quantum = proc->pprio;
            }

            proc->counter = proc->quantum;
            proc->goodness = proc->counter + proc->pprio;
        }
    }
}
