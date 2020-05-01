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
#include "cbmc.h"

BaseType_t prvCopyDataToQueue( Queue_t * const pxQueue, const void *pvItemToQueue, const BaseType_t xPosition );

void harness(){
	QueueHandle_t xQueue = xUnconstrainedQueueBoundedItemSize(10);

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


	if( xQueue ){
		void *pvItemToQueue = pvPortMalloc(xQueue->uxItemSize);
		if( !pvItemToQueue )
		{
			xQueue->uxItemSize = 0;
		}
		if(xQueue->uxItemSize == 0)
		{
			xQueue->uxQueueType = nondet_int8_t();
		}
		BaseType_t xPosition;
		prvCopyDataToQueue( xQueue, pvItemToQueue, xPosition );
	}

}
