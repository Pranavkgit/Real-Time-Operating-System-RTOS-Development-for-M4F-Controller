// Kernel functions
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
#include "mm.h"
#include "kernel.h"
#include "shell_commands.h"
#include "asm.h"
#include "Strings.h"
#include "uart0.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

//CPU Time

uint32_t load_seconds = 1000;
uint8_t track_buffer = 0;


// control
bool priorityScheduler = true;    // priority (true) or round-robin (false)
bool priorityInheritance = false; // priority inheritance for mutexes
bool preemption = true;          // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES   16

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: initialize systick for 1ms system timer
void initsystick(void)
{
    NVIC_ST_RELOAD_R = 39999;
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
}

void init_widetimer(void)
{
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R0;
    _delay_cycles(3);
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;
    WTIMER0_CFG_R = TIMER_CFG_32_BIT_TIMER;
    WTIMER0_TAMR_R|= TIMER_TAMR_TACDIR;
}


void initRtos(void)
{
    initsystick();
    init_widetimer();
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
uint8_t rtosScheduler(void)
{
    if(priorityScheduler)
    {
        uint8_t Max_Priority=0xFF;
        static uint8_t ptask = 0xFF;
        //setting up the priority
        uint8_t i;
        for(i=0;i<MAX_TASKS;i++)
        {
            if(tcb[i].state == STATE_READY && tcb[i].priority < Max_Priority)
            {
                    Max_Priority = tcb[i].priority;
            }
        }
        //Scheduling the priority based task
        bool ok =  false;
        while(!ok)
        {
            ptask ++;
            if(ptask >= MAX_TASKS)
            {
                ptask =0;
            }
            if((tcb[ptask].state == STATE_READY) && (tcb[ptask].priority == Max_Priority))
            {
                ok = true;
            }
        }
        return ptask;
    }
    else{
        bool ok;
        static uint8_t task = 0xFF;
        ok = false;
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY);
        }
        return task;
    }
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn

void startRtos(void)
{
    uint32_t *psp = (uint32_t *) 0x20008000;
    setPSP(psp);
    setASP();
    UnprivilegedMode();
    __asm(" SVC #0");
}

// REQUIRED:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}
            tcb[i].state = STATE_READY;
            tcb[i].pid = fn;
            tcb[i].priority = priority;
            tcb[i].srd = 0;
            tcb[i].dynamic_size = 0;

            tcb[i].current_time = 0;
            tcb[i].elapsed_time = 0;

            //setting up name
            int idx_track=0;
            while(name[idx_track]!='\0')
            {
                tcb[i].name[idx_track]=name[idx_track];
                idx_track++;
            }
            tcb[i].name[idx_track]='\0';

            tcb[i].size = stackBytes;
            //setting up SRD bits
            void *baseAdd = mallocFromHeap(stackBytes);
            uint64_t srdBitMask = createNoSramAccessMask();
            addSramAccessWindow(&srdBitMask, baseAdd, stackBytes);
            tcb[i].srd = srdBitMask;

            //setting up stack
            void *psp =(void*) ((uint32_t)baseAdd + stackBytes);
            tcb[i].sp = psp;
            tcb[i].spInit = baseAdd;
            uint32_t *ptr =(uint32_t*) tcb[i].sp; // track writes

            *(--ptr) = 0x61000000; //Value of Xpsr
            *(--ptr) = (uint32_t) fn; //value of PC
            *(--ptr) = 0x000000FF; //Value of LR
            *(--ptr) = 0x00000012; //Value of r12
            *(--ptr) = 0x00000003; //Value of r3
            *(--ptr) = 0x00000002; //Value of r2
            *(--ptr) = 0x00000001; //Value of r1
            *(--ptr) = 0x00000000; //Value of r0

            *(--ptr) = 0x00000004; //Value of r4
            *(--ptr) = 0x00000005; //Value of r5
            *(--ptr) = 0x00000006; //Value of r6
            *(--ptr) = 0x00000007; //Value of r7
            *(--ptr) = 0x00000008; //Value of r8
            *(--ptr) = 0x00000009; //Value of r9
            *(--ptr) = 0x00000010; //Value of r10
            *(--ptr) = 0x00000011; //Value of r11

            *(--ptr) = 0xFFFFFFFD; //EXEC_RETURN

            tcb[i].sp = ptr;

            taskCount++; // increment task count
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
    __asm(" SVC #15");
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    __asm(" SVC #13");
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm(" SVC #17");
}

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm(" SVC #1");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    __asm(" SVC #2");
}

// REQUIRED: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm(" SVC #3");
}

// REQUIRED: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm(" SVC #4");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm(" SVC #5");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm(" SVC #6");
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    int idx;
    for(idx=0;idx<taskCount;idx++)
    {
        if(tcb[idx].state == STATE_DELAYED)
        {
            tcb[idx].ticks --;
            if(!tcb[idx].ticks)
            {
                tcb[idx].state = STATE_READY;
            }
        }
    }

    if(preemption)
    {
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
    }

    if (load_seconds > 0)
    {
        load_seconds--;
    }

    else if (load_seconds == 0)
    {
        load_seconds = 1000;
        uint8_t i;
        for (i = 0; i < taskCount; i++)
        {
            if(track_buffer == 0)
            {
                tcb[i].elapsed_time = 0;
            }
            else if(track_buffer == 1)
            {
                tcb[i].current_time = 0;
            }
        }
        if(track_buffer == 0)
        {
            track_buffer =1;
        }
        else if(track_buffer == 1)
        {
            track_buffer = 0;
        }
    }

}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
__attribute__((naked)) void pendSvIsr(void)
{
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;
    //saving current task
    SaveTask();
    tcb[taskCurrent].sp = (void *) getPSP();
    if(track_buffer == 0)
    {
        tcb[taskCurrent].current_time = tcb[taskCurrent].current_time + WTIMER0_TAV_R;
    }
    else
    {
        tcb[taskCurrent].elapsed_time = tcb[taskCurrent].elapsed_time + WTIMER0_TAV_R;
    }

    //Restoring next task
    taskCurrent = rtosScheduler();
    uint64_t restore_srd = tcb[taskCurrent].srd;
    applySramAccessMask(restore_srd);
    void *psp = tcb[taskCurrent].sp;
    setPSP((uint32_t *) psp);
    WTIMER0_TAV_R = 0;
    WTIMER0_CTL_R |= TIMER_CTL_TAEN;
    RestoreCurrentTask();
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    uint32_t *sp   =(uint32_t *) getPSP();
    uint32_t *pc   = (uint32_t *) *(sp+6);
    uint8_t *offset = (uint8_t *)pc;
    offset = offset - 2;
    pc = (uint32_t*) offset;
    uint8_t svcNum =(uint8_t) (*pc & 0xFF);

    switch(svcNum)
    {
        case 0:
        {
            taskCurrent = rtosScheduler();
            uint64_t restore_srd = tcb[taskCurrent].srd;
            applySramAccessMask(restore_srd);
            void *psp = tcb[taskCurrent].sp;
            setPSP((uint32_t *) psp);
            RestoreCurrentTask();
            break;
        }
        case 1:
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        case 2:
        {
            uint32_t arg = get_arg0();
            tcb[taskCurrent].ticks = arg;
            tcb[taskCurrent].state = STATE_DELAYED;
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 3:    //LOCK
        {
            uint8_t arg =(uint8_t) get_arg0();
            if(!mutexes[arg].lock)
            {
                tcb[taskCurrent].mutex = arg;
                mutexes[arg].lock = true;
                mutexes[arg].lockedBy = taskCurrent;
            }
            else if(mutexes[arg].queueSize < MAX_MUTEX_QUEUE_SIZE)
            {
                tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
                tcb[taskCurrent].mutex = arg;
                mutexes[arg].processQueue[mutexes[arg].queueSize] = taskCurrent;
                mutexes[arg].queueSize++;
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 4:  //UNLOCK
        {
            uint8_t arg = (uint8_t) get_arg0();
            if(mutexes[arg].lockedBy == taskCurrent)
            {
                if(mutexes[arg].queueSize > 0)
                {
                    tcb[mutexes[arg].processQueue[0]].state = STATE_READY;
                    mutexes[arg].lockedBy = mutexes[arg].processQueue[0];
                    mutexes[arg].lock = true;
                    uint8_t i;
                    for(i=0;i<(mutexes[arg].queueSize);i++)
                    {
                        mutexes[arg].processQueue[i] = mutexes[arg].processQueue[i+1]; //Moving up the task
                    }
                    mutexes[arg].queueSize --;
                }
                else{
                    mutexes[arg].lock = false;
                }
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 5:   //WAIT
        {
            uint8_t arg = (uint8_t) get_arg0();
            if(semaphores[arg].count > 0)
            {
                semaphores[arg].count--;
                break;
            }
            else
            {
                    tcb[taskCurrent].semaphore = arg;
                    semaphores[arg].processQueue[semaphores[arg].queueSize] = taskCurrent;
                    semaphores[arg].queueSize ++;
                    tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            }
            break;
        }
        case 6:   //POST
        {
            uint8_t arg = (uint8_t) get_arg0();
            if(semaphores[arg].queueSize > 0)
            {
                tcb[semaphores[arg].processQueue[0]].state = STATE_READY;
                uint8_t i;
                for(i=0;i<(semaphores[arg].queueSize-1);i++)
                {
                    semaphores[arg].processQueue[i] = semaphores[arg].processQueue[i+1]; //moving up the task
                }
                semaphores[arg].queueSize--;
            }
            else{
                semaphores[arg].count++;
            }
            break;
        }
        case 7: //Reboot
        {
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            break;
        }
        case 8: //Scheduling Priority
        {
            priorityScheduler = true;
            putsUart0("Scheduling: Priority Mode\n");
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 9: //Scheduling round robin
        {
            priorityScheduler = false;
            putsUart0("Scheduling: Round Robin \n");
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 10: //Preempt ON
        {
            preemption = true;
            putsUart0("Preempt ON\n");
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 11: //preempt OFF
        {
            preemption = false;
            putsUart0("Preempt OFF \n");
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 12:  //pidof
        {
            char *Task_Name = (char*) get_arg0();
            //Finding the task
            uint8_t i;
            for(i=0;i<MAX_TASKS;i++)
            {
                char check_str[16];
                uint8_t x=0;
                char ch;
                while(tcb[i].name[x]!='\0')
                {
                    int ascii = (int) tcb[i].name[x];
                    if(ascii>=65 && ascii<=90)
                    {
                        ascii+=32;
                        ch = (char) ascii;
                        check_str[x] = ch;
                    }
                    else{
                        ch = (char) ascii;
                        check_str[x] = ch;
                    }
                    x++;
                }
                check_str[x] ='\0';
                if(!cmp_str(check_str,Task_Name))
                {
                    uint32_t pid =(uint32_t) tcb[i].pid;
                    char str1[10];
                    itoa(pid, str1);
                    putsUart0(str1);
                    putsUart0("\n");
                    break;
                }
            }
            break;
        }
        case 13:  //Kill thread or STOP Thread
        {
            uint32_t pid_arg = (uint32_t) get_arg0();
            uint8_t task_check;
            for(task_check=0;task_check<MAX_TASKS;task_check++)
            {
                uint32_t tcb_pid = (uint32_t) tcb[task_check].pid;
                if(pid_arg == tcb_pid)
                {
                    //clearing the task from mutex queue
                    uint8_t mutex_arg = tcb[task_check].mutex;
                    if(mutexes[mutex_arg].lockedBy == task_check)
                    {
                        mutexes[mutex_arg].lock = false;
                        mutexes[mutex_arg].lockedBy = mutexes[mutex_arg].processQueue[0];
                        tcb[mutexes[mutex_arg].processQueue[0]].state = STATE_READY;
                        uint8_t j;
                        for(j=0;j<(mutexes[mutex_arg].queueSize);j++)
                        {
                            mutexes[mutex_arg].processQueue[j] = mutexes[mutex_arg].processQueue[j+1]; //move up the tasks
                        }
                        mutexes[mutex_arg].queueSize --;
                    }
                    if(mutexes[mutex_arg].queueSize>0 && tcb[task_check].state == STATE_BLOCKED_MUTEX) //tcb[i].mutex --> 0
                    {
                        uint8_t i;
                        for(i=0;i<mutexes[mutex_arg].queueSize;i++)
                        {
                            if(mutexes[mutex_arg].processQueue[i] == task_check)
                            {
                                uint8_t j;
                                for(j=i;j<(mutexes[mutex_arg].queueSize-1);j++)
                                {
                                    mutexes[mutex_arg].processQueue[j] =mutexes[mutex_arg].processQueue[j+1]; //move up the tasks
                                }
                                mutexes[mutex_arg].queueSize --;
                                break;
                            }
                        }
                    }

                    //clearing the task from semaphore queue
                    uint8_t semaphore_arg = tcb[task_check].semaphore;
                    if(tcb[task_check].state == STATE_BLOCKED_SEMAPHORE)
                    {
                        uint8_t i;
                        for(i=0;i<semaphores[semaphore_arg].queueSize;i++)
                        {
                            if(semaphores[semaphore_arg].processQueue[i] == task_check)
                            {
                                uint8_t j;
                                for(j=i;j<(semaphores[semaphore_arg].queueSize);j++)
                                {
                                    semaphores[semaphore_arg].processQueue[j] =semaphores[semaphore_arg].processQueue[j+1]; //move up the tasks
                                }
                                semaphores[semaphore_arg].queueSize --;
                                break;
                            }
                        }
                    }

                    freeToHeap(tcb[task_check].spInit);
                    tcb[task_check].mutex = 0;
                    tcb[task_check].semaphore = 0;
                    tcb[task_check].ticks = 0;
                    tcb[task_check].state = STATE_STOPPED;
                    char final[10];
                    char str1[10];
                    itoa(pid_arg, str1);
                    str_cat(str1, "STOPPED", final);
                    putsUart0(final);
                    putsUart0("\n");
                }
            }
            break;
        }
        case 14:  //PKILL
        {
            char *kill_process = (char *) get_arg0();
            uint8_t task_check;
            for(task_check=0;task_check<MAX_TASKS;task_check++)
            {
                char process[16];
                char ch;
                uint8_t idx=0;
                while(tcb[task_check].name[idx]!='\0')
                {
                    ch = tcb[task_check].name[idx];
                    int ascii =(int) ch;
                    if( ascii >= 65 && ascii <=90)
                    {
                        ascii +=32;
                    }
                    ch = (char) ascii;
                    process[idx] = ch;
                    idx++;
                }
                process[idx] = '\0';
                if(!cmp_str(process,kill_process))
                {
                    //clearing the task from mutex queue
                    uint8_t mutex_arg = tcb[task_check].mutex;
                    if(mutexes[mutex_arg].lockedBy == task_check)
                    {
                        mutexes[mutex_arg].lock = false;
                        mutexes[mutex_arg].lockedBy = mutexes[mutex_arg].processQueue[0];
                        tcb[mutexes[mutex_arg].processQueue[0]].state = STATE_READY;
                        uint8_t j;
                        for(j=0;j<(mutexes[mutex_arg].queueSize);j++)
                        {
                            mutexes[mutex_arg].processQueue[j] = mutexes[mutex_arg].processQueue[j+1]; //move up the tasks
                        }
                        mutexes[mutex_arg].queueSize --;
                    }
                    if(mutexes[mutex_arg].queueSize>0 && tcb[task_check].state == STATE_BLOCKED_MUTEX) //tcb[i].mutex --> 0
                    {
                        uint8_t i;
                        for(i=0;i<mutexes[mutex_arg].queueSize;i++)
                        {
                            if(mutexes[mutex_arg].processQueue[i] == task_check)
                            {
                                uint8_t j;
                                for(j=i;j<(mutexes[mutex_arg].queueSize-1);j++)
                                {
                                    mutexes[mutex_arg].processQueue[j] =mutexes[mutex_arg].processQueue[j+1]; //move up the tasks
                                }
                                mutexes[mutex_arg].queueSize --;
                                break;
                            }
                        }
                    }

                    //clearing the task from semaphore queue
                    uint8_t semaphore_arg = tcb[task_check].semaphore;
                    if(tcb[task_check].state == STATE_BLOCKED_SEMAPHORE)
                    {
                        uint8_t i;
                        for(i=0;i<semaphores[semaphore_arg].queueSize;i++)
                        {
                            if(semaphores[semaphore_arg].processQueue[i] == task_check)
                            {
                                uint8_t j;
                                for(j=i;j<(semaphores[semaphore_arg].queueSize);j++)
                                {
                                    semaphores[semaphore_arg].processQueue[j] =semaphores[semaphore_arg].processQueue[j+1]; //move up the tasks
                                }
                                semaphores[semaphore_arg].queueSize --;
                                break;
                            }
                        }
                    }
                    tcb[task_check].state = STATE_STOPPED;
                    freeToHeap(tcb[task_check].spInit);
                    tcb[task_check].mutex = 0;
                    tcb[task_check].semaphore = 0;
                    tcb[task_check].ticks = 0;
                    char *kill = "killed";
                    char final_str[10];
                    str_cat(kill_process,kill,final_str);
                    putsUart0(final_str);
                    putsUart0("\n");
                    break;
                }
            }
            break;
        }
        case 15: //RESTART THREAD
        {
            uint32_t restart_pid = (uint32_t) get_arg0();
            uint8_t i;
            for(i=0;i<MAX_TASKS;i++)
            {
                uint32_t tcb_pid =(uint32_t) tcb[i].pid;
                if( tcb_pid == restart_pid)
                {
                    uint32_t stackBytes = tcb[i].size;
                    void *baseAdd = mallocFromHeap(stackBytes);
                    uint64_t srdBitMask = createNoSramAccessMask();
                    addSramAccessWindow(&srdBitMask, baseAdd, stackBytes);
                    tcb[i].srd = srdBitMask;

                    //setting up stack
                    void *psp =(void*) ((uint32_t)baseAdd + stackBytes);
                    tcb[i].sp = psp;
                    tcb[i].spInit = baseAdd;

                    uint32_t *ptr =(uint32_t*) tcb[i].sp; // track writes
                    *(--ptr) = 0x61000000; //Value of Xpsr
                    *(--ptr) = (uint32_t) tcb[i].pid; //value of PC
                    *(--ptr) = 0x000000FF; //Value of LR
                    *(--ptr) = 0x00000012; //Value of r12
                    *(--ptr) = 0x00000003; //Value of r3
                    *(--ptr) = 0x00000002; //Value of r2
                    *(--ptr) = 0x00000001; //Value of r1
                    *(--ptr) = 0x00000000; //Value of r0

                    *(--ptr) = 0x00000004; //Value of r4
                    *(--ptr) = 0x00000005; //Value of r5
                    *(--ptr) = 0x00000006; //Value of r6
                    *(--ptr) = 0x00000007; //Value of r7
                    *(--ptr) = 0x00000008; //Value of r8
                    *(--ptr) = 0x00000009; //Value of r9
                    *(--ptr) = 0x00000010; //Value of r10
                    *(--ptr) = 0x00000011; //Value of r11

                    *(--ptr) = 0xFFFFFFFD; //EXEC_RETURN

                    tcb[i].sp = ptr;
                    tcb[i].state = STATE_READY;

                    char final[10];
                    char str1[10];
                    itoa(restart_pid, str1);
                    str_cat(str1, "Restarted", final);
                    putsUart0(final);
                    putsUart0("\n");
                }
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 16:  //IPCS
        {
            //R0 - Argument 0 and R1 for Argument 1
            mutexes_information *mutex_info =(mutexes_information *) get_arg0();
            uint8_t i;
            for(i=0;i<MAX_MUTEXES;i++)
            {
                uint8_t idx1=0;
                while(tcb[mutexes[i].lockedBy].name[idx1]!='\0')
                {
                    mutex_info[i].lockedBy[idx1] = tcb[mutexes[i].lockedBy].name[idx1];
                    idx1++;
                }
                mutex_info[i].lockedBy[idx1]='\0';
                mutex_info[i].queueSize = mutexes[i].queueSize;
                uint8_t j;
                for(j=0;j<mutexes[i].queueSize;j++)
                {
                    uint8_t idx=0;
                    while(tcb[mutexes[i].processQueue[j]].name[idx] != '\0')
                    {
                        mutex_info[i].task_inqueue[j][idx] = tcb[mutexes[i].processQueue[j]].name[idx];
                        idx++;
                    }
                    mutex_info[i].task_inqueue[j][idx]='\0';
                }
                uint32_t *psp =(uint32_t *) getPSP();
                semaphores_information *semaphore_info = (semaphores_information *) (*(psp+1));
                uint8_t k;
                for(k=0;k<MAX_SEMAPHORES;k++)
                {
                    semaphore_info[k].count = semaphores[k].count;
                    semaphore_info[k].queueSize = semaphores[k].queueSize;
                    uint8_t j;
                    for(j=0;j<semaphores[k].queueSize;j++)
                    {
                        uint8_t idx=0;
                        while(tcb[semaphores[k].processQueue[j]].name[idx] != '\0')
                        {
                            semaphore_info[k].task_inqueue[j][idx] = tcb[semaphores[k].processQueue[j]].name[idx];
                            idx++;
                        }
                        semaphore_info[k].task_inqueue[j][idx]='\0';
                    }
                }
            }
            break;
        }
        case 17: //SET PRIORITY
        {
            uint8_t i;
            uint32_t pid =(uint32_t) get_arg0();
            for(i=0;i<MAX_TASKS;i++)
            {
                uint32_t task_pid =(uint32_t) tcb[i].pid;
                if(pid == task_pid)
                {
                    uint32_t *psp =(uint32_t *) getPSP();
                    uint32_t priority = (uint32_t) *(psp+1);
                    tcb[i].priority = priority;
                    putsUart0("Priority Set\n");
                }
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 18: //Kernel Restart Thread
        {
            char *kill_process = (char *) get_arg0();
            uint8_t task_check;
            for(task_check=0;task_check<MAX_TASKS;task_check++)
            {
                char process[16];
                char ch;
                uint8_t idx=0;
                while(tcb[task_check].name[idx]!='\0')
                {
                    ch = tcb[task_check].name[idx];
                    int ascii =(int) ch;
                    if( ascii >= 65 && ascii <=90)
                    {
                        ascii +=32;
                    }
                    ch = (char) ascii;
                    process[idx] = ch;
                    idx++;
                }
                process[idx] = '\0';
                if(!cmp_str(process,kill_process))
                {
                    uint32_t stackBytes = tcb[task_check].size;
                    void *baseAdd = mallocFromHeap(stackBytes);
                    uint64_t srdBitMask = createNoSramAccessMask();
                    addSramAccessWindow(&srdBitMask, baseAdd, stackBytes);
                    tcb[task_check].srd = srdBitMask;

                    //setting up stack
                    void *psp =(void*) ((uint32_t)baseAdd + stackBytes);
                    tcb[task_check].sp = psp;
                    tcb[task_check].spInit = baseAdd;

                    uint32_t *ptr =(uint32_t*) tcb[task_check].sp; // track writes
                    *(--ptr) = 0x61000000; //Value of Xpsr
                    *(--ptr) = (uint32_t) tcb[task_check].pid; //value of PC
                    *(--ptr) = 0x000000FF; //Value of LR
                    *(--ptr) = 0x00000012; //Value of r12
                    *(--ptr) = 0x00000003; //Value of r3
                    *(--ptr) = 0x00000002; //Value of r2
                    *(--ptr) = 0x00000001; //Value of r1
                    *(--ptr) = 0x00000000; //Value of r0

                    *(--ptr) = 0x00000004; //Value of r4
                    *(--ptr) = 0x00000005; //Value of r5
                    *(--ptr) = 0x00000006; //Value of r6
                    *(--ptr) = 0x00000007; //Value of r7
                    *(--ptr) = 0x00000008; //Value of r8
                    *(--ptr) = 0x00000009; //Value of r9
                    *(--ptr) = 0x00000010; //Value of r10
                    *(--ptr) = 0x00000011; //Value of r11

                    *(--ptr) = 0xFFFFFFFD; //EXEC_RETURN

                    tcb[task_check].sp = ptr;
                    tcb[task_check].state = STATE_READY;
                    putsUart0("Restarted\n");
                    break;
                }
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case 19: //PS INFO
        {
            ps_information *ps_info =(ps_information *) get_arg0();
            uint32_t sum_elapsed=0;
            uint32_t sum_current = 0;
            uint64_t cpuTime = 0;
            uint8_t i = 0;
            for(i = 0;i<MAX_TASKS;i++)
            {
                if(track_buffer == 0)
                {
                    cpuTime = tcb[i].elapsed_time;
                    sum_elapsed =sum_elapsed + cpuTime;
                }
                else if(track_buffer == 1)
                {
                    cpuTime = tcb[i].current_time;
                    sum_current =sum_current + cpuTime;
                }
            }
            uint8_t j;
            for(j = 0;j<MAX_TASKS;j++)
            {
                ps_info[j].task = j;
                ps_info[j].pid = (uint32_t) tcb[j].pid;
                uint8_t idx1=0;
                while(tcb[j].name[idx1]!='\0')
                {
                    ps_info[j].name[idx1] = tcb[j].name[idx1];
                    idx1++;
                }
                ps_info[j].name[idx1]='\0';

                if(track_buffer == 0)
                {
                    cpuTime = tcb[j].elapsed_time;
                    cpuTime = ((cpuTime*10000) / sum_elapsed);
                    ps_info[j].cputime = cpuTime;
                }
                else if(track_buffer == 1)
                {
                    cpuTime = tcb[j].current_time;
                    cpuTime = ((cpuTime*10000) / sum_current);
                    ps_info[j].cputime = cpuTime;
                }
            }
            break;
        }
        case 20: //MALLOC WRAPPER
        {
            uint32_t size = (uint32_t) get_arg0();
            uint32_t *base_Address =(uint32_t *) mallocFromHeap(size);
            //Update in the tcb struct
            tcb[taskCurrent].dynamic_size = size;
            uint32_t *psp=(uint32_t*) getPSP();
            *psp =(uint32_t) base_Address;
            uint64_t srdBitMask = createNoSramAccessMask();
            addSramAccessWindow(&srdBitMask, base_Address, size);
            applySramAccessMask(srdBitMask);
            break;
        }
        case 21:
        {
            mem_information *mem_info =(mem_information *) get_arg0();
            uint8_t i;
            for(i=0;i<MAX_TASKS;i++)
            {
                uint8_t idx1=0;
                while(tcb[i].name[idx1]!='\0')
                {
                    mem_info[i].name[idx1] = tcb[i].name[idx1];
                    idx1++;
                }
                mem_info[i].name[idx1]='\0';

                mem_info[i].baseadd = (uint32_t) tcb[i].spInit;

                mem_info[i].size = tcb[i].size;

                mem_info[i].dynamic = tcb[i].dynamic_size;
            }

        }
    }
}


