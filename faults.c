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
#include "faults.h"
#include "Strings.h"
#include "uart0.h"
#include "asm.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    uint32_t pid =32;
    char final_str[100];
    char str[10];
    char *str1 = "MPU Fault in Process";
    itoa(pid,str);
    str_cat(str1,str,final_str);
    putsUart0(final_str);
    putsUart0("\n");
    uint32_t *psp =(uint32_t *) getPSP();
    uint32_t *msp =(uint32_t *) getMSP();
    uint32_t fault_status =(uint32_t) NVIC_FAULT_STAT_R;
    uint32_t Data_Address =(uint32_t) NVIC_MM_ADDR_R;

    str_cat("PSP:",tohex((uint32_t) psp),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    str_cat("MSP:", tohex((uint32_t) msp),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    str_cat("Fault_Status:", tohex((uint32_t) fault_status),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    str_cat("Data Address:", tohex((uint32_t) Data_Address),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    //process stack dump
    uint32_t r0   = *psp;
    uint32_t r1  = *(psp+1);
    uint32_t r2  = *(psp+2);
    uint32_t r3  = *(psp+3);
    uint32_t r12 = *(psp+4);
    uint32_t LR = *(psp+5);
    uint32_t PC = *(psp+6);
    uint32_t xPSR = *(psp+7);
    putsUart0(str_cat("R0:", tohex(r0),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R1:", tohex(r1),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R2:", tohex(r2),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R3:", tohex(r3),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R12:",tohex(r12),final_str));
    putsUart0("\n");
    putsUart0(str_cat("LR:", tohex(LR),final_str));
    putsUart0("\n");
    putsUart0(str_cat("Offending Instruction:", tohex(PC),final_str));
    putsUart0("\n");
    putsUart0(str_cat("xPSR:", tohex(xPSR),final_str));
    putsUart0("\n");

    while(1);
}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    uint32_t pid =32;
    char *str1 = "Bus Fault in Process";
    char final_str[100];
    char str[10];
    itoa(pid,str);
    str_cat(str1,str,final_str);
    putsUart0(final_str);
    putsUart0("\n");

    uint32_t *psp =(uint32_t *) getPSP();
    uint32_t *msp =(uint32_t *) getMSP();
    uint32_t fault_status =(uint32_t) NVIC_FAULT_STAT_R;
    uint32_t hfault =(uint32_t) NVIC_HFAULT_STAT_R;

    str_cat("PSP:",tohex((uint32_t) psp),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    str_cat("MSP:", tohex((uint32_t) msp),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    str_cat("Fault_Status:", tohex((uint32_t) fault_status),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    str_cat("Hard Fault:", tohex((uint32_t) hfault),final_str);
    putsUart0(final_str);
    putsUart0("\n");

    //process stack dump
    uint32_t r0   = *psp;
    uint32_t r1  = *(psp+1);
    uint32_t r2  = *(psp+2);
    uint32_t r3  = *(psp+3);
    uint32_t r12 = *(psp+4);
    uint32_t LR = *(psp+5);
    uint32_t PC = *(psp+6);
    uint32_t xPSR = *(psp+7);
    putsUart0(str_cat("R0:", tohex(r0),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R1:", tohex(r1),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R2:", tohex(r2),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R3:", tohex(r3),final_str));
    putsUart0("\n");
    putsUart0(str_cat("R12:",tohex(r12),final_str));
    putsUart0("\n");
    putsUart0(str_cat("LR:", tohex(LR),final_str));
    putsUart0("\n");
    putsUart0(str_cat("offending Instruction", tohex(PC),final_str));
    putsUart0("\n");
    putsUart0(str_cat("xPSR:", tohex(xPSR),final_str));
    putsUart0("\n");

    while(1);
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    uint32_t pid =32;
    char final_str[100];
    char str[10];
    char *str1 = "Bus Fault in Process";
    itoa(pid,str);
    str_cat(str1,str,final_str);
    putsUart0(final_str);
    putsUart0("\n");
    while(1);
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    uint32_t pid =32;
    char final_str[100];
    char str[10];
    char *str1 = "Usage Fault in Process";
    itoa(pid,str);
    str_cat(str1,str,final_str);
    putsUart0(final_str);
    putsUart0("\n");
    NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_DIV0; //clear the div by 0 fault.
    while(1);
}

