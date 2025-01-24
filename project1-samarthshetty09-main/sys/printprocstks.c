#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

unsigned long fetchStackPointer(int pid) {
    if (pid == currpid) {
        unsigned long sp;
        asm("movl %%esp, %0" : "=r" (sp));
        return sp;
    } else {
        return proctab[pid].pesp;
    }
}

void printprocstks(int priority) {
    kprintf("void printprocstks(int priority)\n");
    int i;
    for (i = 0; i < NPROC; i++) {
        struct pentry *proc = &proctab[i];

        if (proc->pstate != PRFREE && proc->pprio > priority) {
            unsigned long esp = fetchStackPointer(i);

             kprintf("Process [%s]\n", proc->pname);
            kprintf("    pid: %d\n", i);
            kprintf("    priority: %d\n", proc->pprio);
            kprintf("    base: 0x%08X\n", proc->pbase);
            kprintf("    limit: 0x%08X\n", proc->plimit);
            kprintf("    len: %d\n", proc->pstklen);
            kprintf("    pointer: 0x%08X\n", esp);
        }
    }
}
