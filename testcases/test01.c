
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(void *), XXp2(void *);

int tm_pid = -1;

int testcase_main()
{
    int status, pid1, pid2, kidpid;

    tm_pid = getpid();

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: XXp1 should run promptly, as it is high priority; XXp2 should run only *after* we have started the second join(), because it is low priority.\n");

    pid1 = spork("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("Phase 1A TEMPORARY HACK: Manually switching to the first XXp1()\n");
    TEMP_switchTo(pid1);
    USLOSS_Console("testcase_main(): after spork of child %d -- you should not see this until XXp1 has completed.\n", pid1);

    pid2 = spork("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 5);
    USLOSS_Console("testcase_main(): after spork of child %d -- you should see this before XXp2 runs.\n", pid2);

    USLOSS_Console("testcase_main(): performing first join\n");
    kidpid = join(&status);
    if (kidpid != pid1 || status != 3)
    {
        USLOSS_Console("ERROR: kidpid %d status %d\n", kidpid,status);
        USLOSS_Halt(1);
    }
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    USLOSS_Console("testcase_main(): performing second join -- you should see this before XXp2 runs.\n");
    USLOSS_Console("Phase 1A TEMPORARY HACK: Manually switching to the second XXp1()\n");
    TEMP_switchTo(pid2);
    kidpid = join(&status);
    if (kidpid != pid2 || status != 5)
    {
        USLOSS_Console("ERROR: kidpid %d status %d\n", kidpid,status);
        USLOSS_Halt(1);
    }
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    return 0;
}

int XXp1(void *arg)
{
    int i;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    for(i = 0; i < 100; i++)
        ;

    quit_phase_1a(3, tm_pid);
}

int XXp2(void *arg)
{
    USLOSS_Console("XXp2(): started\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);
    quit_phase_1a(5, tm_pid);
}

