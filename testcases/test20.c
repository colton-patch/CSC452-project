/*
 * Check if -1 is returned when there are no more process slots.
 * Attempt to start MAXPROC + 2 processes; i.e., 52 processes
 * Process table has 50 slots. testcase_main and sentinel occupy two of
 *    the slots.  Thus, 48 new processes will start, with error
 *    messages about the last 4 attempts.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(void *);

int   tm_pid = -1;

int testcase_main()
{
    int i, pid1;

    tm_pid = getpid();

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Attempt to create MAXPROC+2 processes (without calling join() on any of them).  This will work many times but eventually fail because all of the procTable slots are full.  We will only print out info about the failed ones.\n");

    USLOSS_Console("testcase_main(): start %d processes\n", MAXPROC+2);

    for (i = 0; i < MAXPROC+2; i++)
    {
        pid1 = spork("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
        if (pid1 < 0)
            USLOSS_Console("testcase_main(): spork() failed: i=%d, pid is %d.\n", i,pid1);
        else
        {
            USLOSS_Console("Phase 1A TEMPORARY HACK: Manually switching to the recently created XXp1()\n");
            TEMP_switchTo(pid1);
        }
    }

    dumpProcesses();

    USLOSS_Console("testcase_main(): Calling join() on %d processes.  The number of 'failed' calls here should be the same as the number of 'failed' spork() calls.\n", MAXPROC+2);
    for (i = 0; i < MAXPROC + 2; i++)
    {
        int status_ignored;
        pid1 = join(&status_ignored);
        if (pid1 < 0)
            USLOSS_Console("testcase_main(): join() failed: i=%d, pid is %d.\n", i,pid1);
    }

    dumpProcesses();

    return 0;
}

int XXp1(void *arg)
{
    quit_phase_1a(2, tm_pid);
}

