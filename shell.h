// Shell functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef SHELL_H_
#define SHELL_H_

#include "kernel.h"
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void shell(void);

typedef struct mutex
{
    char lockedBy[16];
    uint8_t queueSize;
    char task_inqueue[MAX_MUTEX_QUEUE_SIZE][16];
} mutexes_information;

// semaphore
typedef struct semaphore
{
    uint8_t count;
    uint8_t queueSize;
    char task_inqueue[MAX_SEMAPHORE_QUEUE_SIZE][16];
} semaphores_information;

typedef struct ps
{
    uint8_t task;
    uint32_t pid;
    char name[10];
    uint32_t cputime;
}ps_information;

typedef struct meminfo
{
    char name[10];
    uint32_t baseadd;
    uint32_t size;
    uint32_t dynamic;
}mem_information;

#endif
