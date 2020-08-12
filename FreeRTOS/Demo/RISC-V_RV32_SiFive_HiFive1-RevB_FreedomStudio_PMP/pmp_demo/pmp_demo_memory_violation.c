/*
 * FreeRTOS Kernel V10.3.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */
/* FreeRTOS kernel includes. */
#include <FreeRTOS.h>

#include "pmp_initialization.h"
#include "pmp_demo_memory_violation.h"

/*-----------------------------------------------------------*/
int lCounter = 0;

static void prvIncGlobalCounter( void )
{
	lCounter++;
}

/*-----------------------------------------------------------*/

/* Get a memory block from anywhere in RAM.
 * .data section in this case, but doesn't matter.
 * This buffer stores hex representation of prvIncGlobalCounter().
 *
 * To confirm the hex values are still correct after code modification,
 * objdump elf file. When reading objdump result, per RISC-V specification --
 * "instructions are stored in memory as a sequence of 16-bit little-endian
 * parcels, regardless of memory system endianness." */
uint16_t pxInstructionBuffer[17] = {0x1141, 0xc622, 0x0800, 0x37b7, 0x8000, 0xa783,
									0x6087, 0x8713, 0x0017, 0x37b7, 0x8000, 0xa423,
									0x60e7, 0x0001, 0x4432, 0x0141, 0x8082};


void pmp_demo_memory_violation_m_mode( void )
{
	void (*pFn) (void) = (void (*)(void) )pxInstructionBuffer;
	uint16_t uTemp;

	/* Call function to increment global counter. */
	prvIncGlobalCounter();

	/* Execute from RAM is allowed before PMP initialization.
	 * The global counter will be incremented by executing from RAM. */
	pFn();

	/* Initialize PMP to guard against execution in RAM. */
	pmp_initialization_M_mode_only();
	vPortInitInterruptHandler();

	/* Function call is executed as normal. */
	prvIncGlobalCounter();

	/* Execution from RAM in M-mode is NOT allowed with this PMP configuration.
	 * Execution will be trapped in interrupt handler set up above.
	 *
	 * Note:
	 * Set the break point at the next line after p_fn(), and step over p_fn().
	 * (Do not step disassembly, as chip enters debug mode.)
	 * When in interrupt handler, could confirm the address in ra register is
	 * the address of the line after p_fn().*/
	pFn();

	/* Read/write access to RAM is allowed.
	 * In order to run below, please comment out the execution from RAM above. */
	uTemp = pxInstructionBuffer[0];
	pxInstructionBuffer[0] = 0;

	/* To avoid unused variable warning. */
	(void )uTemp;
}

/*-----------------------------------------------------------*/

void pmp_demo_memory_violation_u_mode( void )
{
	void (*pFn) (void) = (void (*)(void) )pxInstructionBuffer;
	uint16_t uTemp;

	BaseType_t xUserModeSupported;

	/* Confirm U-mode is implemented on this MCU. */
	xUserModeSupported = vPortIsUserModeSupported();
	configASSERT( xUserModeSupported == pdTRUE );

	/* Call function to increment global counter. */
	prvIncGlobalCounter();

	/* Execute from RAM is allowed before PMP initialization.
	 * The global counter will be incremented by executing from RAM. */
	pFn();

	/* Initialize PMP. */
	pmp_initialization_U_mode_support();
	vPortInitInterruptHandler();

	/* Drop privilege mode. PMP entries are configured, so only matching addresses
	 * can be accessed from U-mode from now on. */
	portSWITCH_TO_USER_MODE();

	/* Function call is executed as normal. */
	prvIncGlobalCounter();

	/* Read/write access to RAM is allowed before task environment is setup.
	 * Note that after scheduler starts, the PMP rule to allow access to RAM
	 * shall be swapped with fine grained task stack only access. */
	uTemp = pxInstructionBuffer[0];
	pxInstructionBuffer[0] = 0;

	/* Access to M-mode CSR is now allowed in U-mode.
	 * Execution will be trapped in interrupt handler set up above.*/
	volatile uint32_t mstatus;
	__asm volatile ( "csrr %0, mstatus" : "=r" ( mstatus ) );

	/* To avoid unused variable warning. */
	(void )uTemp;
}
