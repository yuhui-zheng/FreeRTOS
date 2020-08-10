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

/* Example PMP setting
 *
 * SiFive HiFive1 Rev1 B01 development board memory map partially looks like below.
 *
 * |-------------------|
 * |    0x2000_0000    |
 * |                   |    E31 ITIM (8 KiB)
 * |    0x3FFF_FFFF    |    Attribute: Read, Write, eXecute, Atomics
 * |-------------------|
 * |                   |
 * |      ......       |
 * |                   |
 * |-------------------|
 * |    0x2000_0000    |
 * |                   |    QSPI 0 Flash (512 MiB)
 * |    0x3FFF_FFFF    |    Attribute: Read, eXecute, Cacheable
 * |-------------------|
 * |                   |
 * |      ......       |
 * |                   |
 * |-------------------|
 * |    0x8000_0000    |
 * |                   |    E31 DTIM (16 KiB) (RAM)
 * |    0x8000_FFFF    |    Attribute: Read, Write, eXecute, Atomics
 * |-------------------|
 *
 * We'll use this map to discuss strategies to prevent common threats like
 * code injection, data corruption, task execution environment isolation, and etc.
 *
 * This particular device has both M-mode and U-mode implemented.
 * Though given RISC-V is an open ISA, two example configurations are given in
 * this file --
 * 1. an example configuration for devices with M-mode only.
 * 2. an example configuration for devices with both M-mode and U-mode.
 *
 * For devices having M-mode only (usually simple embedded systems), the best one
 * can do is probably to ensure not executing from RAM. Code can be placed in
 * Flash, which memory range is usually with eXecute attribute but not Write attribute,
 * and executed in place.
 *
 * For devices having both M-mode and U-mode (usually secure embedded systems),
 * more can be done. Such as:
 * - Each task stack can be guarded to allow access from the owning task only.
 * - Task stacks are Read and Write only.
 * - Tasks are executing in user mode, and kernel calls are made through call gate.
 * This way, kernel is isolated from user, so that the attack surface is limited,
 * and tasks are protected against corruption caused by other tasks.
 */


/* FreeRTOS includes. */
#include <FreeRTOS.h>

/* Freedom metal driver includes. */
#include <metal/pmp.h>

#include "pmp_initialization.h"

/*
 * Format base address for NAPOT address matching.
 *
 * NAPOT -- Naturally aligned power-of-two region, >= 8 bytes.
 * In additional to this function, the underlying linker script used
 * shall also have at least ALIGN(8) for the segment(s) to be protected.
 *
 * Examples for NAPOT address matching:
 * 8-byte:  pmpaddr yyyy....yyy0
 * 16-byte: pmpaddr yyyy....yy01
 * 32-byte: pmpaddr yyyy....y011
 * ...
 */
static size_t prvFormatPmpAddrMatchNapot( size_t ulBaseAddress, uint32_t ulNapotSize );

/*
 * Format base address for TOR address matching.
 */
static size_t prvFormatPmpAddrMatchTor( size_t ulBaseAddress );

/* Setup PMP config handle.
 * pmpxcfg[7] 	L: PMP entry locked.
 * pmpxcfg[4:3] A: PMP entry address matching mode.
 * pmpxcfg[2]	X: executable.
 * pmpxcfg[1]	W: writable.
 * pmpxcfg[0]	R: readable.
 */
static void prvPmpAccessConfig( struct metal_pmp_config * xPmpConfigHandle,
								enum metal_pmp_locked L,
								enum metal_pmp_address_mode A,
								int X,
								int W,
								int R );

/*-----------------------------------------------------------*/


static size_t prvFormatPmpAddrMatchNapot( size_t ulBaseAddress, uint32_t ulNapotSize )
{
size_t ulTempAddress;

	/* Drop the bottom two bits, since:
	   1- each PMP address register encodes bits [33: 2] of a 34-bit physical
	      address for RV32.
	   2- PMP addresses are 4-byte aligned. */
	ulTempAddress = ulBaseAddress >> 2;

	/* Clear the bit corresponding with alignment */
	ulTempAddress &= ~( ulNapotSize >> 3 );

	/* Set the bits up to the alignment bit */
	ulTempAddress |= ( ( ulNapotSize >> 3 ) - 1 );

	return ulTempAddress;
}
/*-----------------------------------------------------------*/


static size_t prvFormatPmpAddrMatchTor( size_t ulBaseAddress )
{
	/* Drop the bottom two bits, since:
	   1- each PMP address register encodes bits [33: 2] of a 34-bit physical
	      address for RV32.
	   2- PMP addresses are 4-byte aligned. */
	return ( ulBaseAddress >> 2 );
}

/*-----------------------------------------------------------*/


static void prvPmpAccessConfig( struct metal_pmp_config * xPmpConfigHandle,
								enum metal_pmp_locked L,
								enum metal_pmp_address_mode A,
								int X,
								int W,
								int R )
{
	/* Since this is not intended to be an API, do not check inputs here.
	 * L, A bits -- see enum definition.
	 * X, W, R bits -- 1 to grant access, 0 to clear access. */
	xPmpConfigHandle->L = L;
	xPmpConfigHandle->A = A;
	xPmpConfigHandle->X = X;
	xPmpConfigHandle->W = W;
	xPmpConfigHandle->R = R;

	return;
}

/*-----------------------------------------------------------*/

void pmp_initialization_M_mode_only( void )
{
	struct metal_pmp *pxPMP;
	struct metal_pmp_config xPmpConfig;

	size_t ulPmpBaseAddress;

	int iStatus;

	/* Setup physical memory protection. */
	pxPMP = metal_pmp_get_device();
	configASSERT( pxPMP );

	metal_pmp_init( pxPMP );

	/* Mark entire RAM region as R/W only, and lock it. So that the rule applies
	 * to M-mode as well. */
	ulPmpBaseAddress = prvFormatPmpAddrMatchNapot( 0x80000000, 0x4000 );
	prvPmpAccessConfig( &xPmpConfig, METAL_PMP_LOCKED, METAL_PMP_NAPOT, 0, 1, 1 );
	iStatus = metal_pmp_set_region( pxPMP, 0, xPmpConfig, ulPmpBaseAddress );
	configASSERT( iStatus == 0 );
}

/*-----------------------------------------------------------*/

void pmp_initialization_U_mode_support( void )
{
struct metal_pmp *pxPMP;
struct metal_pmp_config xPmpConfig;

size_t ulPmpBaseAddress;
uint32_t ulSize;

int iStatus;

extern uint32_t _common_function_start;

	/* Since PMP setting is of security concern and done before kernel loading,
	 * assert on any error. */

	/* Setup physical memory protection. */
	pxPMP = metal_pmp_get_device();
	configASSERT( pxPMP );

	metal_pmp_init( pxPMP );

	/* Grant U-mode access to .text section.
	 * FreeRTOS kernel API call gates are located at the beginning of .text --
	 * [__syscalls_flash_start__, __syscalls_flash_end__], where U-mode has access
	 * to. FreeRTOS kernel API implementations are in a separate section
	 * .privileged_functions -- [_privileged_function_start, +0x8000], where U-mode
	 * does not have access to. Privilege is bumped only for API calls made through
	 * call gate. */
	ulSize = 0x10000;
	ulPmpBaseAddress = prvFormatPmpAddrMatchNapot( (size_t)&_common_function_start, ulSize);
	prvPmpAccessConfig( &xPmpConfig, METAL_PMP_UNLOCKED, METAL_PMP_NAPOT, 1, 0, 1 );
	iStatus = metal_pmp_set_region( pxPMP, 0, xPmpConfig, ulPmpBaseAddress );
	configASSERT( iStatus == 0 );

	/* Temporary grant U-mode access to entire RAM.
	 * This is to allow access to main stack before task environment is setup. */
	ulSize = 0x4000;
	ulPmpBaseAddress = prvFormatPmpAddrMatchNapot( 0x80000000, ulSize );
	prvPmpAccessConfig( &xPmpConfig, METAL_PMP_UNLOCKED, METAL_PMP_NAPOT, 0, 1, 1 );
	iStatus = metal_pmp_set_region( pxPMP, 1, xPmpConfig, ulPmpBaseAddress );
	configASSERT( iStatus == 0 );

}

