#ifndef SHELL_COMMANDS_H_
#define SHELL_COMMANDS_H_

#include<stdio.h>
#include<stdbool.h>
#include<inttypes.h>
#include "shell.h"

void reboot(void);
void ps(ps_information ps_info[]);
void ipcs(mutexes_information mutex_info[],semaphores_information semaphore_info[]);
void kill_pid(uint32_t pid);
void pkill(char proc_name[]);
void preempt(bool on);
void sched(bool prio_on);
void pidof(char process[]);
void proc_check(char *proc_name);
void meminfo(mem_information mem_info[]);


#endif /* SHELL_COMMANDS_H_ */
