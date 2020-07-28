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

/* Initialize physical memory protection.
 *
 * It is recommended to setup memory protection as early as possible, so
 * pmp_initialization() could be called either before loading kernel (bootloader)
 * or starting kernel scheduler.
 *
 * The helper functions provided are assuming RV32. The PMP settings are based
 * on provided linker script metal.pmp.lds.
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

void pmp_initialization( void )
{
struct metal_pmp *pxPMP;
struct metal_pmp_config xPmpConfig;

size_t ulPmpBaseAddress;

int iStatus;

extern uint32_t _privileged_data_start;
extern uint32_t _privileged_function_start;
extern uint32_t _common_function_start;
extern uint32_t _common_data_end;


	/* Since PMP setting is of security concern and done before kernel loading,
	 * assert on any error. */

	/* Setup physical memory protection. */
	pxPMP = metal_pmp_get_device();
	configASSERT( pxPMP );

	metal_pmp_init( pxPMP );

	/* Kernel functions:
	Full access to M-mode, no access to U-mode.
	.privileged_functions section takes 0x4a2e bytes.
	Thus with NAPOT alignment set block size to be 0x8000 bytes. */
	ulPmpBaseAddress = prvFormatPmpAddrMatchNapot( (size_t)&_privileged_function_start, 0x8000 );
	prvPmpAccessConfig( &xPmpConfig, METAL_PMP_UNLOCKED, METAL_PMP_NAPOT, 0, 0, 0 );
	iStatus = metal_pmp_set_region( pxPMP, 0, xPmpConfig, ulPmpBaseAddress );
	configASSERT( iStatus == 0 );

	/* .text section:
	For both M-mode and U-mode, R/X access only.
	.text is in flash and address range [0x2000_0000, 0x3FFFF_FFFF] has memory
	attribute Read/eXecute/Cacheable (no Write attribute). Thus, no harm is
	done even without PMP. The protection is more to catch anomaly instead of
	assuring no change to code at runtime. The exception handler could simply
	recover from this violation without any other action.

	Also note that every time code is modified, needs to check whether the
	section alignment is still correct. To be specific, all .text addresses
	need to fit in one PMP entry. */
	ulPmpBaseAddress = prvFormatPmpAddrMatchNapot( (size_t)&_common_function_start, 0x10000 );
	prvPmpAccessConfig( &xPmpConfig, METAL_PMP_LOCKED, METAL_PMP_NAPOT, 1, 0, 1 );
	iStatus = metal_pmp_set_region( pxPMP, 1, xPmpConfig, ulPmpBaseAddress );
	configASSERT( iStatus == 0 );

	/* Kernel data:
	Full access to M-mode, no access to U-mode.
	.privilege_data section takes 0x1a8 bytes.
	Thus with NAPOT alignment set block size to be 512 bytes. */
	ulPmpBaseAddress = prvFormatPmpAddrMatchNapot( (size_t)&_privileged_data_start, 0x200 );
	prvPmpAccessConfig( &xPmpConfig, METAL_PMP_UNLOCKED, METAL_PMP_NAPOT, 0, 0, 0 );
	iStatus = metal_pmp_set_region( pxPMP, 2, xPmpConfig, ulPmpBaseAddress );
	configASSERT( iStatus == 0 );

	/* .data and .bss sections:
	For both M-mode and U-mode, R/W access only.
	This PMP entry HAS TO be placed RIGHT AFTER "kernel data". AND
	these sections HAVE TO be contiguous -- .privilege_data, .data, .bss
	Since:
	- RAM size is very limited. Using NAPOT/NA4 address matching results
	  significant waste. Thus, use TOR address matching.
	- When TOR address matching is used, access to address within range
	  pmpaddr[i-1] <= y <= pmpaddr[i] is allowed. Also, since address matching
	  starts from the lowest numbered PMP entry, M-mode access to .privilege_data
	  matches previous entry's privilege configuration. M/U-mode access to .data
	  and .bss falls to this PMP entry. */
	ulPmpBaseAddress = prvFormatPmpAddrMatchTor( (size_t)&_common_data_end );
	prvPmpAccessConfig( &xPmpConfig, METAL_PMP_LOCKED, METAL_PMP_TOR, 0, 1, 1 );
	iStatus = metal_pmp_set_region( pxPMP, 3, xPmpConfig, ulPmpBaseAddress );
	configASSERT( iStatus == 0 );

}

