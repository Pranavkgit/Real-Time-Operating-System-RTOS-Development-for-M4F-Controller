#include<stdio.h>
#include<stdbool.h>
#include<inttypes.h>
#include "uart0.h"
#include "gpio.h"
#include "Strings.h"
#include "tm4c123gh6pm.h"
#include "shell.h"

void reboot(void)
{
    __asm(" SVC #7");
}

void ps(ps_information ps_info[])
{
    __asm(" SVC #19");
}

void ipcs(mutexes_information mutex_info[],semaphores_information semaphore_info[])
{
    __asm(" SVC #16");
}

void kill_pid(uint32_t pid)
{
    __asm(" SVC #13");
}

void pkill(char proc_name[])
{
    __asm(" SVC #14");
}

void preempt(bool on)
{
    if(on)
    {
        __asm(" SVC #10");
    }
    else{
        __asm(" SVC #11");
    }
}

void sched(bool prio_on)
{
    if(prio_on)
    {
        __asm(" SVC #8");
    }
    else
    {
        __asm(" SVC #9");
    }
}

void pidof(char process[])
{
    __asm(" SVC #12");
}

void proc_check(char *proc_name)
{
    __asm(" SVC #18");
}

void meminfo(mem_information mem_info[])
{
    __asm(" SVC #21");
}
