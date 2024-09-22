/*
 * Authors: Colton Patch, Ping Tontrasathien
 */

#include <phase1.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// prototypes
void startFuncWrapper(void);
int startFuncInit(void);
void checkForKernelMode(void);
unsigned int disableInterrupts(void);
void dispatcher(void);

// structure for a process control block. Contains PID, name, priority, current context, the process' 
// start function and argument, and pointers to its parent, youngest child, and next older sibling
struct pcb {
	int pid;
	char name[MAXNAME];
	int priority;
	USLOSS_Context *context;
	int (*startFunc)();
	void *arg;
	struct pcb *parent;
	// each process points to its youngest child, which points to its next older sibling and so on
	struct pcb *youngestChild;
	struct pcb *nextOlderSibling;
};


// global variables
int nextId = 0; // The ID of the next process
struct pcb *pcbTable; // table of PCBs
struct pcb *curProc; // currently running process
char initStack[USLOSS_MIN_STACK];
struct pcb *queue1, *queue2, *queue3, *queue4, *queue5, *queue6; // queues for each priority

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
	pcbTable[0].pid = nextId;
	strcpy(pcbTable[0].name, "init");
	pcbTable[0].priority = 6;
	pcbTable[0].startFunc = &startFuncInit; // init's start function
	pcbTable[0].arg = NULL;
	nextId++;

	// initialize context for init
	USLOSS_ContextInit(pcbTable[0].context, initStack, USLOSS_MIN_STACK, NULL, &startFuncWrapper);

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
}


/*
* int spork(char *name, int(*func)(void *), void *arg, int stackSize, int priority)
*	- creates a child process of the current process
*/
int spork(char *name, int(*func)(void *), void *arg, int stackSize, int priority) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();

	// check for reasonable stack size
	if ( stackSize < USLOSS_MIN_STACK) {
                return -2;
        }

	// check if pcbTable is not full, priority is in range, start function and name is not null, name is not too long
        if ( nextId == MAXPROC || (priority < 1 || priority > 5) || (func == NULL || name == NULL || strlen(name) > MAXNAME) ) {
		return -1;
	}

 	// create a new process
	struct pcb *newProc = malloc(sizeof(struct pcb));
	newProc->pid = nextId;
    	strcpy(newProc->name, name);
    	newProc->priority = priority;
    	newProc->startFunc = func;
   	newProc->arg = arg;
  	newPCB->parent = curProc; // set parent to current process
   	newPCB->nextOlderSibling = curProc->youngestChild; // set older sibling to the youngest child of parent;	
	nextId++;

	// update the youngest child of parent
	curProc->youngestChild = newProc;

	// add to table
	int slot = nextId % MAXPROC;
	pcbTable[slot] = newProc;

	// call dispatcher
	// dispatcher(); nothing happens in phase1a

	// restore interrupts
	USLOSS_PsrSet(prevPsr);

	return newProc->pid;
}


int join(int *status) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();
	
	// check invalid arguments passed to the function
	if ( status == NULL ) {
		return -3;
	}

	// check the process does not have any children
	if ( curProc->youngestChild == NULL ) {
		return -2;
	}

	// fill status
	//status = 	

	// restore interrupts
	USLOSS_PsrSet(prevPsr);
	
	return curProc->youngestChild->pid;
}

void quit_phase_1a(int status, int switchToPid) {
	// make sure in kernel mode and disable interrupts
	checkForKernelMode();
	unsigned int prevPsr = disableInterrupts();

	//

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

/*
* int getpid(void) - returns the PID of the currently running process.
*/
int getpid(void) {
	checkForKernelMode();
	return curProc->pid;
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
void startFuncWrapper(void) {
	int (*startFunc)(void *) = curProc->startFunc;
	void *arg = curProc -> arg;
	int status = (*startFunc)(arg);
	quit(status);
}

/*
* void startFuncInit(void) - The start function for the init process. It calls spork() to
*	create the testcase_main process, then repeatedly calls join() until it returns -2.
*/
int startFuncInit(void) {
	spork("testcase_main", &testcase_main, NULL, USLOSS_MIN_STACK, 3);

	int joinVal = 0;
	int *joinStatus = 0;
	while (joinVal != -2) {
		joinVal = join(joinStatus);
	}

	USLOSS_Trace("ERROR: init process has no children");
	return 1;
}

/*
* void checkForKernelMode(void) - Halts if not currently in kernel mode 
*/
void checkForKernelMode(void) {
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
unsigned int disableInterrupts(void) {
	unsigned int prevPsr = USLOSS_PsrGet();
	USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT);
	return prevPsr;
}


//void dispatcher() {} for phase1b


