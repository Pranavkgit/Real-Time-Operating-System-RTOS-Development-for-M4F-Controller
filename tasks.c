// Tasks
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
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "tasks.h"
#include "mm.h"
#include "asm.h"

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTA,2 // off-board red LED
#define ORANGE_LED PORTA,3 // off-board orange LED
#define YELLOW_LED PORTA,4 // off-board yellow LED
#define GREEN_LED  PORTE,0 // off-board green LED

#define PB0    PORTC,4
#define PB1    PORTC,5
#define PB2    PORTC,6
#define PB3    PORTC,7
#define PB4    PORTD,6
#define PB5    PORTD,7
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs and pushbuttons
    enablePort(PORTF);
    enablePort(PORTE);
    enablePort(PORTA);
    enablePort(PORTC);
    enablePort(PORTD);
    _delay_cycles(3);
    selectPinPushPullOutput(PORTF,2);
    selectPinPushPullOutput(PORTE,0);
    selectPinPushPullOutput(PORTA,2);
    selectPinPushPullOutput(PORTA,3);
    selectPinPushPullOutput(PORTA,4);

    selectPinDigitalInput(PB0);
    selectPinDigitalInput(PB1);
    selectPinDigitalInput(PB2);
    selectPinDigitalInput(PB3);
    selectPinDigitalInput(PB4);

    setPinCommitControl(PB5);
    selectPinDigitalInput(PB5);

    enablePinPullup(PB0);
    enablePinPullup(PB1);
    enablePinPullup(PB2);
    enablePinPullup(PB3);
    enablePinPullup(PB4);
    enablePinPullup(PB5);

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    uint8_t value = 0;
    if(getPinValue(PB0) == 0)
    {
        value |= 1;  //PB0
    }
    if(getPinValue(PB1) == 0)
    {
        value |= 2;  //PB1
    }
    if(getPinValue(PB2) == 0)
    {
        value |= 4;  //PB2
    }
    if(getPinValue(PB3) == 0)
    {
        value |= 8;  //PB3
    }
    if(getPinValue(PB4) == 0)
    {
        value |= 16;  //PB4
    }
    if(getPinValue(PB5) == 0)
    {
        value |= 32;  //PB5
    }
    return value;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}

void idle2(void)
{
    while(true)
    {
        setPinValue(YELLOW_LED, 1);
        waitMicrosecond(1000);
        setPinValue(YELLOW_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}
void* Malloc_wrapper(uint32_t size)
{
    __asm(" SVC #20");
}


void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    mem = Malloc_wrapper(5000 * sizeof(uint8_t));
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}
