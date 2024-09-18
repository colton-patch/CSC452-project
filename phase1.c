/*
 * Authors: Colton Patch, Ping Tontrasathien
 */

#include <phase1.h>

// structure for a process control block. Contains PID, name, priority, current context, the process' 
// start function and argument, and pointers to its parent, youngest child, and next older sibling
struct pcb {
	int pid;
	char name[MAXNAME];
	int priority;
	USLOSS_Context *context;
	int (*startFunc)(void *);
	void *arg;
	struct pcb *parent;
	// each process points to its youngest child, which points to its next older sibling and so on
	struct pcb *youngestChild;
	struct pcb *nextOlderSibling;
};

// global variables
int curId = 1; // The ID of the next process
struct pcb *pcbTable; // table of PCBs
struct pcb *curProc; // currently running process

/*
* void phase1_init(void) - creates the PCB table and the PCB structure for the 
*	init process.
*/
void phase1_init(void) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();

	// create table
	struct pcb newTable[MAXPROC];
	pcbTable = &newTable[0];

	// make pcb entry for init
	pcbTable[0].pid = curId;
	pcbTable[0].name = &"init";
	pcbTable[0].priority = 6;
	pcbTable[0].startFunc = &startFuncInit; // init's start function
	pcbTable[0].arg = NULL;
	curId++;

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}

int spork(char *name, int(*func)(void *), void *arg, int stacksize, int priority) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();
	

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}

int join(int *status) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();
	

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}

void quit_phase1a(int status, int switchToPid) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();
	

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}

void quit(int status) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();
	

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}

int getpid(void) {
	checkForKernelMode();

}

void dumpProcesses(void) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();
	

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}

void TEMP_switchTo(int pid) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();
	
	// switch to new process with given pid
	int slot = pid % MAXPROC;
	struct pcb *oldProc = curProc;
	curProc = &pcbTable[slot];
	USLOSS_ContextSwitch(oldProc->context, curProc->context);

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}

/*
* void startFuncWrapper(void) - wrapper for any process' start function. It will call the 
* 	start function of the current process, then quit() if that function returns.
*/
static void startFuncWrapper(void) {
	int (*startFunc)(void *) = curProc->startFunc;
	void *arg = curProc -> arg;
	int status = (*startFunc)(arg);
	quit(status);
}

/*
* void startFuncInit(void) - The start function for the init process. It calls spork() to
*	create the testcase_main process, then repeatedly calls join() until it returns -2.
*/
static void startFuncInit(void) {
	int retval = spork("testcase_main", &testcase_main, NULL, USLOSS_MIN_STACK, 3);

	int joinVal = 0;
	int *joinStatus;
	while (joinVal != -2) {
		joinVal = join(joinStatus);
	}

	USLOSS_Trace("ERROR: init process has no children"); 
}

/*
* void checkForKernelMode(void) - Halts if not currently in kernel mode 
*/
static void checkForKernelMode(void) {
	// check if not in kernel mode
	if ( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
		USLOSS_Trace("ERROR: kernel function called from user mode");
		USLOSS_Halt(1);
	}
}

/*
* unsigned int disableInterrupts(void) - sets interrupts to disables and returns the previous state
*	of the PSR.
*/
static unsigned int disableInterrupts(void) {
	unsigned int prevPsr = USLOSS_PsrGet();
	USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT);
	return prevPsr;
}




