/*
 * FreeRTOS memory safety proofs with CBMC.
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "queue_init.h"
#include "tasksStubs.h"
#include "cbmc.h"

/* prvUnlockQueue is going to decrement this value to 0 in the loop.
   We need a bound for the loop. Using 4 has a reasonable performance resulting
   in 3 unwinding iterations of the loop. The loop is mostly modifying a
   data structure in task.c that is not in the scope of the proof. */
#ifndef LOCK_BOUND
	#define LOCK_BOUND 4
#endif

/* This code checks for time outs. This value is used to bound the time out
   wait period. The stub function xTaskCheckForTimeOut used to model
   this wait time will be bounded to this define. */
#ifndef QUEUE_RECEIVE_BOUND
	#define QUEUE_RECEIVE_BOUND 4
#endif

/* If the item size is not bounded, the proof does not finish in a reasonable
   time due to the involved memcpy commands. */
#ifndef MAX_ITEM_SIZE
	#define MAX_ITEM_SIZE 20
#endif

QueueHandle_t xQueue;

/* This method is used to model side effects of concurrency.
   The initialization of pxTimeOut is not relevant for this harness. */
void vTaskInternalSetTimeOutState( TimeOut_t * const pxTimeOut ){
	__CPROVER_assert(__CPROVER_w_ok(&(pxTimeOut->xOverflowCount), sizeof(BaseType_t)), "pxTimeOut should be a valid pointer and xOverflowCount writable");
	__CPROVER_assert(__CPROVER_w_ok(&(pxTimeOut->xTimeOnEntering), sizeof(TickType_t)), "pxTimeOut should be a valid pointer and xTimeOnEntering writable");
}

void harness(){
	/* This is for loop unwinding. */
	vInitTaskCheckForTimeOut(0, QUEUE_RECEIVE_BOUND - 1);

	xQueue = xUnconstrainedQueueBoundedItemSize(MAX_ITEM_SIZE);
	__CPROVER_assume(xQueue);
	
	xQueue->cTxLock = nondet_int8_t();
	xQueue->cRxLock = nondet_int8_t();
	xQueue->uxLength = nondet_UBaseType_t();
	xQueue->uxMessagesWaiting = nondet_UBaseType_t();

	/* This is an invariant checked with a couple of asserts in the code base.
	If it is false from the beginning, the CBMC proofs are not able to succeed*/
	__CPROVER_assume(xQueue->uxMessagesWaiting < xQueue->uxLength);
	xQueue->xTasksWaitingToReceive.uxNumberOfItems = nondet_UBaseType_t();
	xQueue->xTasksWaitingToSend.uxNumberOfItems = nondet_UBaseType_t();
	#if( configUSE_QUEUE_SETS == 1)
		xQueueAddToSet(xQueue, xUnconstrainedQueueSet());
	#endif

	void *pvBuffer = pvPortMalloc( xQueue->uxItemSize );
	__CPROVER_assume( pvBuffer || xQueue->uxItemSize == 0 );

	TickType_t xTicksToWait;
	__CPROVER_assume( xState != taskSCHEDULER_SUSPENDED || xTicksToWait == 0 );

	/* This is for loop unwinding. */
	__CPROVER_assume( xQueue->cTxLock = LOCK_BOUND - 1 );
	__CPROVER_assume( xQueue->cRxLock = LOCK_BOUND - 1 );
	xQueueReceive( xQueue, pvBuffer, xTicksToWait );
}
