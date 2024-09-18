/*
 * Authors: Colton Patch, Ping
 */

#include <phase1.h>

// structure for a process control block. Contains PID, name, priority, process state, current context, 
// the process' start function and argument, and pointers to its parent, youngest child, and next older sibling
struct pcb {
	int pid;
	char name[MAXNAME];
	int priority;
	int processState; // 0-ready, 1-running, 2-blocked
	USLOSS_context *context;
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


void phase1_init(void) {
	// create table
	struct pcb newTable[MAXPROC];
	pcbTable = &newTable[0];

	// make pcb entry for init
	pcbTable[0].pid = curId;
	pcbTable[0].name = "init";
	pcbTable[0].priority = 6;
	pcbTable[0].processState = 0; // ready 
	curId++;
}

int spork(char *name, int(*func)(void *), void *arg, int stacksize, int priority) {
	
}

int join(int *status) {

}

void quit_phase1a(int status, int switchToPid) {

}

void quit(int status) {

}

int getpid(void) {

}

void dumpProcesses(void) {
	
}

void TEMP_switchTo(int pid) {
	int slot = pid % MAXPROC;
	oldProc = curProc;
	curProc = &pcbTable[slot];
	USLOSS_ContextSwitch(oldProc->context, curProc->context);
}

static void startFuncWrapper() {
	int (*startFunc)(void *) = curProc->startFunc;
	void *arg = curProc -> arg;
	int status = (*startFunc)(arg);
	quit(status);
}

static void startFuncInit() {
	//int retval = spork("testcase_main", &testcase_main, NULL, );
}  




