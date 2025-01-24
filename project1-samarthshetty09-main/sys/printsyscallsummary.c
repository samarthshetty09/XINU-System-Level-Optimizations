#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

// Define global variables
struct syscall_stats syscall_table[NPROC][NSYSCALLS];
int syscall_tracing = 0;

extern unsigned long ctr1000;  // Timer variable to track system time in ms

void syscallsummary_start() {
    int i, j;
    syscall_tracing = 1;  // Enable tracing

    // Clear the syscall statistics
    for (i = 0; i < NPROC; i++) {
        for (j = 0; j < NSYSCALLS; j++) {
            syscall_table[i][j].frequency = 0;
            syscall_table[i][j].time = 0;
        }
    }
}

void syscallsummary_stop() {
    syscall_tracing = 0;  // Disable tracing
}

void printsyscallsummary() {
    int i, j;
    char *syscall_names[NSYSCALLS] = {
        "sys_freemem", "sys_chprio", "sys_getpid", "sys_getprio", "sys_gettime", "sys_kill", "sys_receive", "sys_recvclr", "sys_recvtim",
        "sys_resume", "sys_scount", "sys_sdelete", "sys_send", "sys_setdev", "sys_setnok", "sys_screate", "sys_signal", "sys_signaln", 
        "sys_sleep", "sys_sleep10", "sys_sleep100", "sys_sleep1000", "sys_sreset", "sys_stacktrace", "sys_suspend", "sys_unsleep", "sys_wait"
    };

    kprintf("void printsyscallsummary()\n");
    for (i = 0; i < NPROC; i++) {
        int total_calls = 0;
        for (j = 0; j < NSYSCALLS; j++) {
            if (syscall_table[i][j].frequency > 0) {
                if (total_calls == 0) {
                    kprintf("Process [pid:%d]:\n", i);
                }
                total_calls++;

               unsigned long avg_time = syscall_table[i][j].time / syscall_table[i][j].frequency;

                kprintf("    Syscall: %s, count: %d, average execution time: %lu (ms)\n",
                        syscall_names[j], syscall_table[i][j].frequency, avg_time);
            }
        }
    }
}
