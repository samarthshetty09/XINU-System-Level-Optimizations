#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

int prX;
void halt();

/*------------------------------------------------------------------------
*  main  --  user main program
*------------------------------------------------------------------------
*/
prch(c)
char c;
{
    int i;
    sleep(5);	
}
int main()
{	
	long res = 0xaabbccdd;
	res = zfunction(res);
	kprintf("Task 1 0x%08X\n", res);
    kprintf("Task 3 (printsyscallsummary)\n");
    syscallsummary_start();        
   	freemem(0, 100);
	freemem(0, 100);
	freemem(0, 100);
	chprio(1, 20);
	getpid();
	getprio(1);
	gettime(5);
	kill(1);
	// receive();
	// receive();
	// receive();		
	recvclr();
	recvtim(10);
	resume(1);
	scount(1);
	sdelete(1);
	send(1, 2);
	setdev(1, 2, 2);
	setnok(1, 2);
	screate(1);
	signal(1);
	signaln(1, 2);
	sleep(1);
	sleep10(1);
	sleep100(1);
	sleep1000(1);
	sreset(1,4);
	stacktrace(1);
	suspend(1);
	unsleep(1);
	wait(1);		
    syscallsummary_stop();
    printsyscallsummary();


    return 0;
}