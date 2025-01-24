[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/Z24-hGtU)
# Project 2: Process Scheduling

## 1. Objective
The objective of this assignment is to get familiar with the concepts of process management, including process priorities, scheduling, and context switching.

## 2. Readings

- The Xinu source code for functions is in [sys](sys), especially those related to process creation (`create.c`), scheduling (`resched.c`, `resume.c`, `suspend.c`), termination (`kill.c`), changing priority (`chprio.c`), system initialization (`initialize.c`), and other related utilities (e.g., `ready.c`), etc.

## 3. Tasks

In this assignment, you will implement two new scheduling policies, which avoid the *starvation* problem in process scheduling. At the end of this assignment, you will be able to explain the advantages and disadvantages of the two new scheduling policies.

The default scheduler in Xinu schedules processes based on the highest priority. Starvation occurs when two or more processes that are eligible for execution have different priorities. The process with the higher priority gets to execute first, resulting in processes with lower priorities never getting any CPU time unless processes with the higher priority end.

The two scheduling policies that you need to implement, as described below, should address this problem. Note that for each of them, you need to consider how to handle the NULL process, so that this process is selected to run when and only when there are no other ready processes.

For Linux-like scheduling policies, the value of a valid process priority is an integer between 0 to 99, where 99 is the highest priority. Only the NULL process can have priority 0. All others must be at least 1.


1. **Scheduler 1: Exponential Distribution Scheduler (`EXPDISTSCHED`)**

    The first scheduling policy is the exponential distribution scheduler. This scheduler chooses the next process based on a random value that follows the [*exponential distribution*](https://en.wikipedia.org/wiki/Exponential_distribution). When a rescheduling occurs, the scheduler generates a random number with the exponential distribution, and chooses a process with the lowest priority that is greater than the random number. 
    - If the random value is less than the lowest priority in the ready queue, the process with the lowest priority is chosen. 
    - If the random value is greater than or equal to the highest priority in the ready queue, the process with the highest priority is chosen. 
    - When there are processes having the same priority, they should be scheduled in a round-robin way. The processes do not need to run adjacent to each other, just that the same process in that priority level should not be run again until all others have had a chance to run. For example if you have processes A and B with priority 20 and C with priority 10, a valid order might be `A, C, C, B, C, A`, but it should not be `A, C, C, A, C, B`.
    
    (**hint**: The moment you insert a process into the ready queue, the queue is ordered.)

    For example, let us assume that the scheduler uses a random value with the exponential distribution of $λ = 0.1$, and there are three processes A, B, and C, whose priorities are 10, 20, and 30, respectively. When rescheduling happens, if a random value is less than 10, process A will be chosen by the scheduler. If the random value is greater than or equal to 10 and less than 20, process B will be chosen. If the random value is greater than or equal to 20, process C will be chosen. The probability that a process is chosen by the scheduler follows the exponential distribution. As shown in the figure below, the ratio of processes A (priority 10), B (priority 20), and C (priority 30) to be chosen is 0.63 : 0.23 : 0.14. It can be mathematically calculated by the cumulative distribution function $F(x; λ) = 1 - e^{-λx}.$ This results in processes with lower priority values being considered higher priority, which is the desired behavior.

    ![graph](expdist.jpg)
    
    In order to implement an exponential distribution scheduler, you need to implement `expdev()`, which generates random numbers that follow the exponential distribution. The generator can be implemented by $-1/λ * \log(1 - y)$, where $y$ is a random number following the uniform distribution in $[0, 1]$.

    **Implementation**:

    ```c
    double expdev(double lambda) {
        double dummy;
        do
            dummy= (double) rand() / RAND_MAX;
        while (dummy == 0.0);
        return -log(dummy) / lambda;
    }
    ```

    **Implementation Tips**: Since Xinu does not provide any math library (i.e., math.h), you will implement `log()` by yourself. For `log()` implementation, you can use the [Taylor series](https://en.wikipedia.org/wiki/Taylor_series), where $n = 20$, i.e. 20 iterations, can give a reasonable approximation. Values passed to the log function should always be between 0 and 1. Note that Xinu does not offer any format to print out a floating point number, so you can first test your code with gcc in Linux and use it in Xinu. You can also roughly test it in Xinu by casting, e.g. `kprintf("%d", (int) log(0.5))`. You will also have to implement `pow()`. You are recommended to put `expdev()`, `log()`, and `pow()` in `math.c` and add it to `Makefile`. On the other hand, `srand()` and `rand()` functions are offered in [lib/libxc/rand.c](lib/libxc/rand.c) by Xinu.

    ```c
    double log(double x);
    double pow(double x, int y);
    double expdev(double lambda);
    ```

    **Comment**: As the value of $λ$, use 0.1. The mean of the exponential distribution is $1/λ$, so it is 10 for $λ = 0.1$. Since the priority ranges from 0 to 99, 10 can be meaningful. The $λ$ value may be changed for a different scheduling property, but your code will be tested for $λ = 0.1$.

1. **Scheduler 2: Linux-like Scheduler (`LINUXSCHED`)**

    This scheduling algorithm loosely emulates the Linux scheduler in the 2.2 kernel. We consider all the processes "conventional processes" and use the policies of the `SCHED_OTHER` scheduling class within the 2.2 kernel. In this algorithm, the scheduler divides CPU time into continuous epochs. In each *epoch*, every existing process is allowed to execute up to a given *time quantum*, which specifies the maximum allowed time for a process within the current epoch. The time quantum for a process is computed and refreshed at the beginning of each epoch. If a process has used up its quantum during an epoch, it cannot be scheduled until another epoch starts. For example, a time quantum of 10 allows a process to only execute for 10 ticks (10 timer interrupts) within an epoch. On the other hand, a process can be scheduled many times as long as it has not used up its time quantum. For example, a process may invoke `sleep` before using up its quantum, but still be scheduled after waking up within an epoch. The scheduler ends an epoch and starts a new one when all the **runnable** processes (i.e., processes in the ready queue) have used up their time.

    The rules for computing the time quantum for a process are as follows. At the beginning of a new epoch, the scheduler computes the time quantum for **every** existing processes, including the blocked ones. As a result, a blocked process can start in the new epoch when it becomes runnable. 
    - For a process that has never executed or used up its time quantum in the previous epoch, its new quantum value is set to its process priority (i.e., `quantum = priority`). 
    - For a process that has not used up its quantum in the previous epoch, the scheduler allows half of the unused quantum to be carried over to the new epoch. Suppose for each process there is a variable counter describing how many ticks are left from its previous quantum, then the new quantum value is set to `floor(counter / 2) + priority`. For example, a counter of 5 and a priority of 10 produce a new quantum value of 12. 
    
    Any new processes created in the middle of an epoch have to wait until the next epoch to be scheduled. Any priority changes in the middle of an epoch, e.g., through `create()` and `chprio()`, only take effect in the next epoch.

    To schedule processes during each epoch, the scheduler introduces a *goodness value* for each process. This goodness value is essentially a dynamic priority that is updated whenever the scheduler is invoked (**hint**: consider when the scheduler could be invoked). 
    - For a process that has used up its quantum, its goodness value is 0. 
    - For a runnable process, the scheduler considers both the priority and the remaining quantum (recorded by counter) in computing the goodness value: `goodness = counter + priority` (**hint**: when you implement this, you need to consider the rule that any priority changes only take effect in the next epoch). 
    
    The scheduler always schedules a runnable process that has the highest *goodness* value (it uses the round-robin strategy if there are processes with the same goodness value). Unlike the expdistched, processes with a higher priority value are considered higher priority. This is the desired behavior. The scheduled process will keep running without being preempted (**hint**: look into `clkint.S`, also consider how to handle the `NULL` process) until it yields or uses up its time quantum.

    Examples of how processes should be scheduled under this scheduler are as follows:
    - If there are processes P1, P2, P3 with priority 10, 20, 15, the initial time quantum for each process would be P1 = 10, P2 = 20, and P3 = 15, so the maximum CPU time for an epoch would be 10 + 20 + 15 = 45. 
    - The possible schedule for two epochs would be P2, P3, P1, P2, P3, P1, but not: P2, P3, P2, P1, P3, P1.
    - If P1 yields in the middle of its execution (e.g., by invoking sleep) in the first epoch, the time quantum for each process in the second epoch could be P1 = 12, P2 = 20, and P3 = 15, thereby the maximum CPU time would be 12 + 20 + 15 = 47.


### Other Implementation Details

1. `void setschedclass (int sched_class)`

    This function should change the scheduling type to either `EXPDISTSCHED` or `LINUXSCHED`. Scheduling policies will only be set prior to running tests, so there is no need to handle policy switching.

2. `int getschedclass()`
    
    This function should return the scheduling class, which should be either `EXPDISTSCHED` or `LINUXSCHED`.

3. Each of the scheduling class should be defined as a constant:

    ```c
    #define EXPDISTSCHED 1
    #define LINUXSCHED 2
    ```

4. Some of the source files of interest are: `create.c`, `resched.c`, `resume.c`, `suspend.c`, `ready.c`, `proc.h`, `kernel.h`, etc.

## 4. Test Cases

You should use `testmain.c` as your main.c program.

```c
/* testmain.c */

#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <sched.h>
#include <stdio.h>
#include <math.h>

#define LOOP	50

int prA, prB, prC;
int proc_a(char), proc_b(char), proc_c(char);
int proc(char);
volatile int a_cnt = 0;
volatile int b_cnt = 0;
volatile int c_cnt = 0;
volatile int s = 0;

int main() {
    int i;
    int count = 0;
    char buf[8];

    srand(1234);

    kprintf("Please Input:\n");
    while ((i = read(CONSOLE, buf, sizeof(buf))) < 1)
        ;
    buf[i] = 0;
    s = atoi(buf);
    kprintf("Get %d\n", s);

    // EXPDISTSCHED
    if (s < 2) {
        setschedclass(EXPDISTSCHED);
        prA = create(proc_a, 2000, 10, "proc A", 1, 'A');
        prB = create(proc_b, 2000, 20, "proc B", 1, 'B');
        prC = create(proc_c, 2000, 30, "proc C", 1, 'C');
        resume(prA);
        resume(prB);
        resume(prC);
        sleep(10);
        kill(prA);
        kill(prB);
        kill(prC);

        kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
    }
    // LINUXSCHED
    else {
        setschedclass(LINUXSCHED);
        resume(prA = create(proc, 2000, 5, "proc A", 1, 'A'));
        resume(prB = create(proc, 2000, 50, "proc B", 1, 'B'));
        resume(prC = create(proc, 2000, 90, "proc C", 1, 'C'));

        while (count++ < LOOP) {
            kprintf("M");
            for (i = 0; i < 1000000; i++)
                ;
        }
        kprintf("\n");
    }
    // This shutdown function causes Xinu to exit, equivalent to ctrl+a, c, q.
    // This behavior was not present in PA1, and if you would like to have the
    // same behavior as in PA1, you can comment it out.
    shutdown();
    return 0;
}

int proc_a(char c) {
    int i;
    kprintf("Start... %c\n", c);
    b_cnt = 0;
    c_cnt = 0;

    while (1) {
        for (i = 0; i < 10000; i++)
            ;
        a_cnt++;
    }
    return 0;
}

int proc_b(char c) {
    int i;
    kprintf("Start... %c\n", c);
    a_cnt = 0;
    c_cnt = 0;

    while (1) {
        for (i = 0; i < 10000; i++)
            ;
        b_cnt++;
    }
    return 0;
}

int proc_c(char c) {
    int i;
    kprintf("Start... %c\n", c);
    a_cnt = 0;
    b_cnt = 0;

    while (1) {
        for (i = 0; i < 10000; i++)
            ;
        c_cnt++;
    }
    return 0;
}

int proc(char c) {
    int i;
    int count = 0;

    while (count++ < LOOP) {
        kprintf("%c", c);
        for (i = 0; i < 1000000; i++)
            ;
    }
    return 0;
}
```    

#### For Exponential Distribution Scheduler:

Three processes A, B, and C are created with priorities 10, 20, 30. In each process, we keep increasing a variable, so the variable value is proportional to CPU time. Note that the ratio should be close to 0.63 : 0.23 : 0.14, as the three processes are scheduled based on the exponential distribution. You are expected to get similar results as follows:

```
Start... A
Start... B
Start... C

Test Result: A = 982590, B = 370179, C = 206867
```

#### For Linux-like Scheduler:

Three processes A, B, and C are created with priorities 5, 50, and 90. In each process, we print a character ('A', 'B', or 'C') at a certain time interval for LOOP times (LOOP = 50). The main process also prints a character ('M') at the same time interval. You are expected to get similar results as follows:

```
MCCCCCCCCCCCCCBBBBBBBMMMACCCCCCCCCCCCCBB
BBBBBMMMACCCCCCCCCCCCBBBBBBBMMMACCCCCCCC
CCCCBBBBBBBMMBBBBBBBMMMABBBBBBBMMMABBBBB
BBMMMBMMAMMMAMMMMMAMMMAMMMAMMMMMMAMMAMMM
MMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```

There are two primary things to keep in mind when reviewing the output: The order of the letters and the approximate ratio of their count. In the case above, M prints first because it is the only process active for the first epoch. Once the second epoch starts, C runs to completion before any of the other tasks get a chance to run because it is the highest priority, and then does not run again until all other processes have had a chance to run. For the ratios, since 90 is approximately 2x 50, there are approximately 2x as many Cs as there are Bs, and so on. The exact counts do not have to be identical so long as the ratios are approximately correct.

## 5. Additional Questions

Write your answers to the following questions in a file named `PA2Answers.txt` (in simple text). Please place this file in the `sys/` directory and turn it in, along with the above programming assignment.

1. (10 pts) What are the advantages and disadvantages of each of the two scheduling policies? Also, give the advantages and disadvantages of the round-robin scheduling policy originally implemented in Xinu.

2. (10 pts) Describe the way each of the schedulers affects the NULL process.


## 6. Turn-in Instructions

1. Preparation
    - You can write code in main.c to test your procedures, but please note that when we test your programs we will replace the main.c file! Therefore, do not put any functionality in the main.c file.

    - Also, ALL debugging output MUST be turned off before you submit your code.

2. Submission
    - Go to the [compile](compile/) directory and do `make clean`.

    - Add new files to your repository using `git add filepath` command. 

    - Create a directory `TMP_unityid`, where `unityid` is your Unity ID (e.g., `TMP_xinurocks`), under the root of the project, and copy all the files you have modified/created, both .c files and .h files, into the new directory. Then, add the directory to the repository by doing `git add TMP_xinurocks` (which will add all the files under the directory).

    - Check if all the necessary changes have been staged by using `git status`. 

    - Then, commit and push:
        ```shell
        git commit -am 'Done'
        git push
        ```
        You can do commit and push whenever you want, but the final submission must be committed/pushed with the message `Done`. For a final check, go to your repository using a web browser, and check if your changes have been pushed.


## 7. Grading Policy

- (20 pts) The "Additional Questions" above. 

- (50 pts) Can code be compiled and started up correctly?

- (30 pts) 15 pts for each scheduler. 3 test cases (similar to the test cases in `testmain.c` above). 5 pts/testcase. 
