/*
 * Authors: Colton Patch, Ping Tontrasathien
 * phase1.c - Implements phase1a; defines functions for initializing the PCB table, creating
 * 	a new child process, joining dead processes with their parents, and quitting the current process.
 * 	Processes are switched to manually as there is no dispatcher.
 */

#include <phase1.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// prototypes
//
void startFuncWrapper(void);
int startFuncInit(void *);
int testcase_mainWrapper(void *);
int checkForKernelMode(void);
unsigned int disableInterrupts(void);
void restoreInterrupts(unsigned int prevPsr);
//void dispatcher(void);

//
// structure for a process control block. Contains PID, name, priority, current context, the process' 
// start function and argument, and pointers to its parent, youngest child, and next older sibling
//
struct pcb {
	char name[MAXNAME];
	int pid; // -1 if no process
	int priority;
	int state; // 0 = Runnable, 1 = Running, 2 = Terminated
	int status; // return status, NULL if still alive
	int (*startFunc)(void *);
	void *arg;
	struct pcb *parent;
	// each process points to its youngest child, which points to its next older sibling and so on
	struct pcb *youngestChild;
	struct pcb *nextOlderSibling;
	USLOSS_Context *context;
};

//
// global variables
//
int nextId = 1; // The ID of the next process
int numProcs = 0; // number of processes
struct pcb pcbTable[MAXPROC]; // table of PCBs
struct pcb *curProc; // currently running process
char initStack[USLOSS_MIN_STACK]; // stack for init
char *stateArr[3] = {"Runnable", "Running", "Terminated"};
USLOSS_Context initContext; // context for init
// struct pcb *queue1, *queue2, *queue3, *queue4, *queue5, *queue6; // queues for each priority

//
// functions
//

/*
* void phase1_init(void) - creates the PCB table and the PCB structure for the 
*	init process.
*/
void phase1_init(void) {
	// make sure in kernel mode and disable interrupts
	if (checkForKernelMode() == 0) {
		USLOSS_Trace("ERROR: Someone attempted to call phase1_init while in user mode!\n");
		USLOSS_Halt(1);
	}
	unsigned int prevPsr = disableInterrupts();

	// make pcb entry for init
	strcpy(pcbTable[1].name, "init");
	pcbTable[1].pid	=  nextId;
	pcbTable[1].priority = 6;
	pcbTable[1].state = 0;
	pcbTable[1].startFunc = &startFuncInit; // init's start function
	pcbTable[1].arg = NULL;
	pcbTable[1].parent = &pcbTable[0];	
	pcbTable[1].context = &initContext;
	nextId++;

	// set all other entries' pid to -1
	pcbTable[0].pid = -1;
	for (int i = 2; i < MAXPROC; i++) {
		pcbTable[i].pid = -1;
	}

	// initialize context for init
	USLOSS_ContextInit(pcbTable[1].context, initStack, USLOSS_MIN_STACK, NULL, &startFuncWrapper);

	// increment number of processes
	numProcs++;

	// restore interrupts
	restoreInterrupts(prevPsr);
}


/*
* int spork(char *name, int(*func)(void *), void *arg, int stackSize, int priority)
*	- creates a child process of the current process and returns its pid
*	name - name of the new process
*	func - start function of the new process
*	arg - argument for the new process' start function
*	stacksize - size of the stack to be allocated for the new process
*	priority - priority of the new process
*/
int spork(char *name, int(*func)(void *), void *arg, int stackSize, int priority) {
	// make sure in kernel mode and disable interrupts
	if (checkForKernelMode() == 0) {
		USLOSS_Trace("ERROR: Someone attempted to call spork while in user mode!\n");
		USLOSS_Halt(1);
	}
	unsigned int prevPsr = disableInterrupts();

	// check for reasonable stack size
	if ( stackSize < USLOSS_MIN_STACK) {
		return -2;
	}

	// check if pcbTable is not full, priority is in range, start function and name is not null, name is not too long
	if ( numProcs == MAXPROC || (priority < 1 || priority > 5) || (func == NULL || name == NULL || strlen(name) > MAXNAME) ) {
		return -1;
	}

	// get slot in table
	int slot = nextId % MAXPROC;
	while (pcbTable[slot].pid != -1) {
		nextId++;
		slot = nextId % MAXPROC;
	}

 	// define fields
	pcbTable[slot].pid = nextId;
	strcpy(pcbTable[slot].name, name);
	pcbTable[slot].priority = priority;
	pcbTable[slot].startFunc = func;
	pcbTable[slot].arg = arg;
	pcbTable[slot].state = 0;
	pcbTable[slot].parent = curProc; // set parent to current process
	pcbTable[slot].youngestChild = NULL;
	pcbTable[slot].nextOlderSibling = curProc->youngestChild; // set older sibling to the youngest child of parent;	
	nextId++;

	// initialize context
	char *newStack = malloc(sizeof(char) * stackSize);
	pcbTable[slot].context = malloc(sizeof(USLOSS_Context));
	USLOSS_ContextInit(pcbTable[slot].context, newStack, stackSize, NULL, &startFuncWrapper);

	// increment number of processes
	numProcs++;

	// update the youngest child of parent
	if (curProc->youngestChild != NULL) {
		pcbTable[slot].nextOlderSibling = curProc->youngestChild;
	}
	curProc->youngestChild = &pcbTable[slot];

	// call dispatcher
	// dispatcher(); nothing happens in phase1a

	// restore interrupts
	restoreInterrupts(prevPsr);

	return pcbTable[slot].pid;
}

/*
* int join(int *status) - Joins the current process with its dead child then returns that
*	child's PID and stores its status.
*	status - pointer to store the dead child's status in.
*/
int join(int *status) {
	// make sure in kernel mode and disable interrupts
	if (checkForKernelMode() == 0) {
		USLOSS_Trace("ERROR: Someone attempted to call join while in user mode!\n");
		USLOSS_Halt(1);
	}
	unsigned int prevPsr = disableInterrupts();

	// check invalid arguments passed to the function
	if ( status == NULL) {
		return -3;
	}
	
	// check the process does not have any children
	if ( curProc->youngestChild == NULL ) {
		return -2;
	}

	// find dead child
	struct pcb *nextChild = curProc->youngestChild;
	struct pcb *prevChild = NULL;
	while (nextChild->state != 2 && nextChild->nextOlderSibling != NULL) {
		prevChild = nextChild;
		nextChild = nextChild->nextOlderSibling;
	}
	
	// fill status and get pid
	*status = nextChild->status;
	int deadPid = nextChild->pid;

	// remove dead child from list and free the stack
	if (prevChild == NULL) {
		curProc->youngestChild = curProc->youngestChild->nextOlderSibling;
	} 
	else {
		prevChild->nextOlderSibling = nextChild->nextOlderSibling;
	}
	free(nextChild->context);

	// set pid to -1 and decrement number of processes
	nextChild->pid = -1;
	numProcs--;

	// restore interrupts
	restoreInterrupts(prevPsr);
	
	return deadPid;
}

/*
* void quit_phase_1a(int status, int switchToPid) - terminates the current process and switches
*	to the process with the given PID.
*	status - the status of the process when it's main function returns.
*	switchToPid - the PID of the process to switch to next.
*/
void quit_phase_1a(int status, int switchToPid) {
	// make sure in kernel mode and disable interrupts
	if (checkForKernelMode() == 0) {
		USLOSS_Trace("ERROR: Someone attempted to call quit_phase_1a while in user mode!\n");
		USLOSS_Halt(1);
	}
	unsigned int prevPsr = disableInterrupts();

	
	// check that all children have been joined
	if (curProc->youngestChild) {
		USLOSS_Trace("ERROR: Process pid %d called quit() while it still had children.\n", curProc->pid);
		USLOSS_Halt(1);
	}

	// save the status and flag as terminated
	curProc->status = status;
	curProc->state = 2;

	// context switch
	TEMP_switchTo(switchToPid);

	// restore interrupts
	restoreInterrupts(prevPsr);
}

// void quit(int status) {
// 	// make sure in kernel mode and disable interrupts
// 	checkForKernelMode();
// 	unsigned int prevPsr = disableInterrupts();
	
// 	// context switch
// 	//TEMP_switchTO(curProc->parent->pid);

// 	// restore interrupts
// 	USLOSS_PsrSet(prevPsr);
// }

/*
* int getpid(void) - returns the PID of the currently running process.
*/
int getpid(void) {
	if (checkForKernelMode() == 0) {
		USLOSS_Trace("ERROR: Someone attempted to call getpid while in user mode!\n");
		USLOSS_Halt(1);
	}
	return curProc->pid;
}


/*
* void dumpProcesses(void) - prints out process infromation from the process table, in a human-readable format. 
*/
void dumpProcesses(void) {
	// make sure in kernel mode and disable interrupts
	if (checkForKernelMode() == 0) {
		USLOSS_Trace("ERROR: Someone attempted to call dumpProcesses while in user mode!\n");
		USLOSS_Halt(1);
	};
	unsigned int prevPsr = disableInterrupts();

	// header
	USLOSS_Console("%4s %5s  %-17s %-9s %s\n", "PID", "PPID", "NAME", "PRIORITY", "STATE");
	
	// processes
	for (int i = 0; i < MAXPROC; i++) {
		struct pcb p = pcbTable[i];
		if (p.pid != -1) {
			int ppid = (p.pid == 1) ? 0 : p.parent->pid; // make ppid 0 if process it init
			USLOSS_Console("%4d %5d  %-17s %-9d %s", p.pid, ppid, p.name, p.priority, stateArr[p.state]);
			// print the status if terminated
			if (p.state == 2) {
				USLOSS_Console("(%d)", p.status);
			}
			USLOSS_Console("\n");
		}
	}

	// restore interrupts
	restoreInterrupts(prevPsr);
}

/*
* void TEMP_switchTo(int pid) - Context switches to the process with the given PID.
*	pid - PID of the proccess to switch to.
*/
void TEMP_switchTo(int pid) {
	// make sure in kernel mode and disable interrupts
	if (checkForKernelMode() == 0) {
		USLOSS_Trace("ERROR: Someone attempted to call TEMP_switchTo while in user mode!\n");
		USLOSS_Halt(1);
	};
	unsigned int prevPsr = disableInterrupts();
	
	// switch to new process with given pid
	int slot = pid % MAXPROC;
	struct pcb *oldProc = curProc;
	curProc = &pcbTable[slot];
	curProc->state = 1; // set new to Running

	if (pid == 1) { // don't store old proc on first process
		USLOSS_ContextSwitch(NULL, curProc->context);
	} else {
		oldProc->state = (oldProc->state == 2) ? 2 : 0; // set old to Runnable if not terminated
		USLOSS_ContextSwitch(oldProc->context, curProc->context);
	}

	// restore interrupts
	restoreInterrupts(prevPsr);
}

/*
* void startFuncWrapper(void) - wrapper for any process' start function. It will call the 
* 	start function of the current process, then quit() if that function returns.
*/
void startFuncWrapper(void) {
	int (*startFunc)(void *) = curProc->startFunc;
	void *arg = curProc -> arg;
	
	// enable interrupts before calling start function
	unsigned int prevPsr = USLOSS_PsrGet();
	if (USLOSS_PsrSet(prevPsr | USLOSS_PSR_CURRENT_INT) == USLOSS_ERR_INVALID_PSR) {
		USLOSS_Trace("ERROR: Invalid PSR");
		USLOSS_Halt(1);
	}

	// cal start function and quit when it returns
	int status = (*startFunc)(arg);
	quit_phase_1a(status, curProc->parent->pid);
}

/*
* void startFuncInit(void) - The start function for the init process. It calls spork() to
*	create the testcase_main process, then repeatedly calls join() until it returns -2.
*/
int startFuncInit(void *) {
	phase2_start_service_processes();
	phase3_start_service_processes();
	phase4_start_service_processes();
	phase5_start_service_processes();

	spork("testcase_main", &testcase_mainWrapper, NULL, USLOSS_MIN_STACK, 3);
	USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to testcase_main() after using spork() to create it.\n");
	TEMP_switchTo(2); // only for phase1a - manually switch to testcase_main

	// int joinVal = 0;
	// int zero = 0;
	// int *joinStatus = &zero;
	// while (joinVal != -2) {
	// 	joinVal = join(joinStatus);
	// }

	// USLOSS_Trace("ERROR: init process has no children\n");
	return 1;
}

/*
* int testcase_mainWrapper(void *) - wrapper for testcase_main. Calls testcase_main and halts
*	when it returns. Exists to be passed to spork() so the type is compatible.
*/
int testcase_mainWrapper(void *) {
	testcase_main();
	// when testcase_main returns
	USLOSS_Console("Phase 1A TEMPORARY HACK: testcase_main() returned, simulation will now halt.\n");
	USLOSS_Halt(0);

	return 1;
}

/*
* int checkForKernelMode(void) - returns 0 if not in kernel mode
*/
int checkForKernelMode(void) {
	// check if not in kernel mode
	return (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()); 
}

/*
* unsigned int disableInterrupts(void) - sets interrupts to disabled and returns the previous state
*	of the PSR.
*/
unsigned int disableInterrupts(void) {
	unsigned int prevPsr = USLOSS_PsrGet();
	if (USLOSS_PsrSet(prevPsr & ~USLOSS_PSR_CURRENT_INT) == USLOSS_ERR_INVALID_PSR) {
		USLOSS_Trace("ERROR: Invalid PSR");
		USLOSS_Halt(1);
	}
	return prevPsr;
}

/*
* void restoreInterrupts(unsigned int prevPsr) - restores the state of interrupts to its
*	previous value, as passed by prevPsr
*	prevPsr - the previous value of the psr to restore to
*/
void restoreInterrupts(unsigned int prevPsr) {
	if (USLOSS_PsrSet(prevPsr) == USLOSS_ERR_INVALID_PSR) {
		USLOSS_Trace("ERROR: Invalid PSR");
		USLOSS_Halt(1);
	}
}


//void dispatcher() {} for phase1b


