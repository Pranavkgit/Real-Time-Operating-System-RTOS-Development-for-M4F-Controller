// Shell functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "shell.h"
#include "Strings.h"
#include "uart0.h"
#include "kernel.h"
#include "shell_commands.h"


// REQUIRED: Add header files here for your strings functions, ...

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    USER_DATA userdata;
    while(1)
    {
        if(kbhitUart0())
        {
            getsUart0(&userdata);
            parseFields(&userdata);
            if(isCommand(&userdata, "reboot", 0))
            {
                reboot();
                yield();
            }
            else if(isCommand(&userdata, "ps", 0))
            {
                ps_information ps_info[12]={0,};
                ps(ps_info);
                putsUart0("Task\t PID\t Name\t \tCPU TIME\r\n");
                uint8_t i;
                for(i=0;i<=9;i++)
                {
                    char t[10],pid[10],cpu[10];
                    itoa(ps_info[i].task,t);
                    putsUart0(t);
                    putsUart0("\t ");
                    itoa(ps_info[i].pid,pid);
                    putsUart0(pid);
                    putsUart0("\t ");
                    putsUart0(ps_info[i].name);
                    putsUart0("   \t ");
                    itoa(ps_info[i].cputime,cpu);
                    char *final_str = percentage(cpu);
                    putsUart0(final_str);
                    putsUart0("\t ");
                    putsUart0("\r\n");
                }
                putsUart0("\r\n");
                yield();
            }
            else if(isCommand(&userdata, "ipcs", 0))
            {
                mutexes_information mutex_info[MAX_MUTEXES]= {0,};
                semaphores_information semaphore_info[MAX_SEMAPHORES] = {0,};
                //update values from original mutex/semaphore to mutex_information/semaphore_information
                ipcs(mutex_info,semaphore_info);
                //printout the updated values

                uint8_t i;
                putsUart0("Mutexes \n");
                putsUart0("Locked By\t Queue size\t In Queue \t");
                putsUart0("\r\n");
                for(i=0;i<MAX_MUTEXES;i++)
                {
                    char size[10];
                    putsUart0(mutex_info[i].lockedBy);
                    putsUart0("\t ");
                    itoa(mutex_info[i].queueSize,size);
                    putsUart0(size);
                    putsUart0("\t ");
                    putsUart0("\t ");
                    uint8_t j;
                    for(j=0;j<mutex_info[i].queueSize;j++)
                    {
                        putsUart0(mutex_info[i].task_inqueue[j]);
                    }
                    putsUart0("\r\n");
                }

                uint8_t k;
                putsUart0("Semaphores \n");
                putsUart0("count\t Queue size\t InQueue\t");
                putsUart0("\r\n");
                for(k=0;k<MAX_SEMAPHORES;k++)
                {
                    char c[10];
                    char s[10];
                    itoa(semaphore_info[k].count,c);
                    putsUart0(c);
                    putsUart0("\t ");
                    itoa(semaphore_info[k].queueSize,s);
                    putsUart0(s);
                    putsUart0("\t ");
                    putsUart0("\t ");
                    uint8_t j;
                    for(j=0;j<semaphore_info[k].queueSize;j++)
                    {
                        putsUart0(semaphore_info[k].task_inqueue[j]);
                        putsUart0("\n");
                    }
                    putsUart0("\r\n");
                }
                yield();
            }
            else if(isCommand(&userdata, "kill", 1))
            {
                uint32_t pid =(uint32_t) getFieldInteger(&userdata, userdata.fieldPosition[1]);
                kill_pid(pid);
                yield();
            }
            else if(isCommand(&userdata, "pkill", 1))
            {
                char *cmd = getFieldString(&userdata, userdata.fieldPosition[1]);
                char process[20];
                int i=0;
                while(cmd[i]!='\0')
                {
                    process[i]=cmd[i];
                    i++;
                }
                process[i]='\0';
                pkill(process);
            }
            else if(isCommand(&userdata, "preempt", 1))
            {
                char *cmd = getFieldString(&userdata, userdata.fieldPosition[1]);
                if(cmp_str(cmd,"on") == 0)
                {
                    preempt(1);
                }
                else if(cmp_str(cmd,"off")==0)
                {
                    preempt(0);
                }
            }
            else if(isCommand(&userdata, "sched", 1))
            {
                char *cmd = getFieldString(&userdata, userdata.fieldPosition[1]);
                if(cmp_str(cmd,"prio") == 0)
                {
                    sched(1);//priority scheduling
                }
                else if(cmp_str(cmd,"rr")==0)
                {
                    sched(0);   //Round robin scheduling
                }
                yield();
            }
            else if(isCommand(&userdata,"pidof", 1))
            {
                char *cmd = getFieldString(&userdata, userdata.fieldPosition[1]);
                char process[20];
                int i=0;
                while(cmd[i]!='\0')
                {
                    process[i]=cmd[i];
                    i++;
                }
                process[i]='\0';
                pidof(process);
                yield();
            }
            else if(isCommand(&userdata,"meminfo", 0))
            {
                mem_information mem_info[MAX_TASKS];
                meminfo(mem_info);
                putsUart0("Name\t BaseAddress\t size\t DynamicSize\r\n");
                uint8_t i;
                for(i=0;i<10;i++)
                {
                    char size_str[10],dyn[10];
                    putsUart0(mem_info[i].name);
                    putsUart0("   \t ");
                    char *baseadd = tohex(mem_info[i].baseadd);
                    putsUart0(baseadd);
                    putsUart0("\t ");
                    itoa(mem_info[i].size,size_str);
                    putsUart0(size_str);
                    putsUart0("\t ");
                    itoa(mem_info[i].dynamic,dyn);
                    putsUart0(dyn);
                    putsUart0("\t ");
                    putsUart0("\r\n");
                }
                putsUart0("\r\n");
                yield();
            }
            else{
                char *cmd = getFieldString(&userdata, userdata.fieldPosition[0]);
                proc_check(cmd);
            }
        }
        else{
            yield();
        }
    }
}
