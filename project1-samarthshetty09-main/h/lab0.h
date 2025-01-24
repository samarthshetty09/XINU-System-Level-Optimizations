#ifndef LAB0_H
#define LAB0_H

#define NSYSCALLS 27  // Number of implemented system calls
#define NPROC 50      // Maximum number of processes in the system

// Define a structure to hold syscall statistics
struct syscall_stats {
    int frequency;      // How many times the syscall was invoked
    unsigned long time; // Total time taken by the syscall
};

// Declare the global array to store syscall statistics for each process
extern struct syscall_stats syscall_table[NPROC][NSYSCALLS];
extern int syscall_tracing; // A flag to indicate if tracing is enabled
extern unsigned long ctr1000;

// Enumeration for system call IDs
enum syscall_ids {
    SYSCALL_FREEMEM = 0,
    SYSCALL_CHPRIO,
    SYSCALL_GETPID,
    SYSCALL_GETPRIO,
    SYSCALL_GETTIME,
    SYSCALL_KILL,
    SYSCALL_RECEIVE,
    SYSCALL_RECVCLR,
    SYSCALL_RECVTIM,
    SYSCALL_RESUME,
    SYSCALL_SCOUNT,
    SYSCALL_SDELETE,
    SYSCALL_SEND,
    SYSCALL_SETDEV,
    SYSCALL_SETNOK,
    SYSCALL_SCREATE,
    SYSCALL_SIGNAL,
    SYSCALL_SIGNALN,
    SYSCALL_SLEEP,
    SYSCALL_SLEEP10,
    SYSCALL_SLEEP100,
    SYSCALL_SLEEP1000,
    SYSCALL_SRESET,
    SYSCALL_STACKTRACE,
    SYSCALL_SUSPEND,
    SYSCALL_UNSLEEP,
    SYSCALL_WAIT
};

// System call names, mapped to the enumeration values
extern char *syscall_names[NSYSCALLS];

// Function prototypes
void syscallsummary_start();
void syscallsummary_stop();
void printsyscallsummary();

#endif /* LAB0_H */
