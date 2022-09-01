/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include <time.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#define CCM_RAM __attribute__((section(".ccmram")))

// ----------------------------------------------------------------------------

#include "led.h"

#define BLINK_PORT_NUMBER         (3)
#define BLINK_PIN_NUMBER_GREEN    (12)
#define BLINK_PIN_NUMBER_ORANGE   (13)
#define BLINK_PIN_NUMBER_RED      (14)
#define BLINK_PIN_NUMBER_BLUE     (15)
#define BLINK_ACTIVE_LOW          (false)

struct led blinkLeds[4];

// ----------------------------------------------------------------------------
/*-----------------------------------------------------------*/

/*
 * The LED timer callback function.  This does nothing but switch the red LED
 * off.
 */

static void prvOneShotTimerCallback( TimerHandle_t xTimer );
static void prvAutoReloadTimerCallback( TimerHandle_t xTimer );

/*-----------------------------------------------------------*/

/* The LED software timer.  This uses vLEDTimerCallback() as its callback
 * function.
 */
static TimerHandle_t xTimer1 = NULL;
static TimerHandle_t xTimer2 = NULL;
BaseType_t xTimer1Started, xTimer2Started;

/*-----------------------------------------------------------*/
// ----------------------------------------------------------------------------
//
// Semihosting STM32F4 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//


#define queueSize  20
TaskHandle_t Sender1Handle=NULL;
TaskHandle_t Sender2Handle=NULL;
TaskHandle_t ReceiverHandle=NULL;
TaskHandle_t ResetCheckHandle=NULL;
QueueHandle_t MainQueue=0;

TimerHandle_t Sender1TimerHandle;
TimerHandle_t Sender2TimerHandle;
TimerHandle_t ReceiverTimerHandle;

SemaphoreHandle_t sender1Mutex;
SemaphoreHandle_t sender2Mutex;
SemaphoreHandle_t receiverMutex;
SemaphoreHandle_t locker;
SemaphoreHandle_t counterMutex;



unsigned short TransmittedMessages=0;
unsigned short BlockedMessages=0;
unsigned short ReceivedMessages=0;
unsigned short lowerLimit[6]={50,80,110,140,170,200};
unsigned short upperLimit[6]={150,200,250,300,350,400};
unsigned short Tsender1;
unsigned short Tsender2;
unsigned short Treceiver;
unsigned short indexo=0;

char atFirst=1;



void Reset(void *p)
{
	while(1)
	{


		if(ReceivedMessages==500 && atFirst==0)
		{
			indexo++;
			printf("Total number of transmitted messages = %d \n",TransmittedMessages);
			printf("Total number of messages reached the queue = %d \n",TransmittedMessages-BlockedMessages);
			printf("Total number of successfully sent messages = %d \n",ReceivedMessages);
			printf("Total number of blocked messages = %d \n",BlockedMessages);
			TransmittedMessages=0;
			BlockedMessages=0;
			ReceivedMessages=0;
			xQueueReset(MainQueue);
			if(indexo<=5)
			{

				Tsender1=lowerLimit[indexo]+(rand()%(upperLimit[indexo]-lowerLimit[indexo]));
				Tsender2=lowerLimit[indexo]+(rand()%(upperLimit[indexo]-lowerLimit[indexo]));
				xTimerChangePeriod(Sender1TimerHandle,(pdMS_TO_TICKS( Tsender1)),(TickType_t)0);
				xTimerChangePeriod(Sender2TimerHandle,(pdMS_TO_TICKS (Tsender2)),(TickType_t)0);

			}
			else
			{
				printf("Game Over \n");
				xTimerDelete(Sender1TimerHandle,(TickType_t)0);
				xTimerDelete(Sender2TimerHandle,(TickType_t)0);
				xTimerDelete(ReceiverTimerHandle,(TickType_t)0);

				vTaskDelete(ResetCheckHandle);

			}

		}
		else if(atFirst!=0)
		{
			printf("Timer periods initialized\n");
			xQueueReset(MainQueue);
			Tsender1=lowerLimit[indexo]+(rand()%(upperLimit[indexo]-lowerLimit[indexo]));
			Tsender2=lowerLimit[indexo]+(rand()%(upperLimit[indexo]-lowerLimit[indexo]));

			Treceiver=pdMS_TO_TICKS(100);
			atFirst=0;
			return ;
		}
		else
		{

		}

	}
}

void Sender1Task(void *p)
{

	BaseType_t status;
	char Buff[200];
	TickType_t XYZ;

	while(1)
	{
		printf("In sender 1\n");
		XYZ=xTaskGetTickCount();
			sprintf(Buff,"Time is %d \n",XYZ);
		if(xSemaphoreTake(sender1Mutex,(TickType_t)0xFFFFFFFF))
		{
			status=xQueueSend(MainQueue,(void*)Buff,(TickType_t)0);
			TransmittedMessages++;
			if(xSemaphoreTake(locker,(TickType_t)0xFFFFFFFF))
			{
				if(status==0)
				{
					BlockedMessages++;

				}
				else
				{

				}

			}
			xSemaphoreGive(locker);
		}
	}
}
void Sender2Task(void*p)
{

	BaseType_t status;
	char Buff[200];
	TickType_t XYZ;

	while(1)
	{
		printf("In sender 2\n");
		XYZ=xTaskGetTickCount();
		sprintf(Buff,"Time is %d \n",XYZ);
		if(xSemaphoreTake(sender2Mutex,(TickType_t)0xFFFFFFFF))
		{
			status=xQueueSend(MainQueue,(void*)Buff,(TickType_t)0);
			TransmittedMessages++;
			if(xSemaphoreTake(locker,(TickType_t)0xFFFFFFFF))
			{
				if(status==0)
				{
					BlockedMessages++;

				}
				else
				{

				}

			}
			xSemaphoreGive(locker);
		}

	}
}
static void Timer1Callback()
{
	printf("In timer1 call back\n");
	xSemaphoreGive(sender1Mutex);

		Tsender1=lowerLimit[indexo]+(rand()%(upperLimit[indexo]-lowerLimit[indexo]));


		xTimerChangePeriod(Sender1TimerHandle,(pdMS_TO_TICKS( Tsender1)),(TickType_t)0);


}
static void Timer2Callback()
{
	printf("In timer 2 callback\n");
	xSemaphoreGive(sender2Mutex);


		Tsender2=lowerLimit[indexo]+(rand()%(upperLimit[indexo]-lowerLimit[indexo]));


		xTimerChangePeriod(Sender2TimerHandle,(pdMS_TO_TICKS (Tsender2)),(TickType_t)0);

}

static void ReceiverCallback()
{
	printf("In receiver callback\n");
	xSemaphoreGive(receiverMutex);
}

void ReceiverTask(void*p)
{
	printf("In receiver task\n");
	char Buff[200];
	BaseType_t status=0;
	while(1)
	{
	if(xSemaphoreTake(receiverMutex,(TickType_t)0xFFFFFFFF))
	{
		if(MainQueue!=0)
		status=xQueueReceive(MainQueue,(void*)Buff,(TickType_t)5);
		if(status==1)
		{
			ReceivedMessages++;
			printf("The message is : %s \n",Buff);
		}
		else{}
	}
	}

}


// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

int
main(void)
{
	// Add your code here.

	srand(time(NULL));
	BaseType_t t1=0,t2=0,t3=0;
		char Buffer[200];
		MainQueue=xQueueCreate(queueSize,sizeof(Buffer));

		sender1Mutex=xSemaphoreCreateBinary();
		sender2Mutex=xSemaphoreCreateBinary();
		locker=xSemaphoreCreateMutex();
		receiverMutex=xSemaphoreCreateBinary();


		xTaskCreate(Sender1Task,"S1",2000,(void*)0,0,&Sender1Handle);
		xTaskCreate(Sender2Task,"S2",2000,(void*)0,0,&Sender2Handle);
		xTaskCreate(ReceiverTask,"R1",2000,(void*)0,1,&ReceiverHandle);
		xTaskCreate(Reset,"ResetTask",2000,(void*)0,0,&ResetCheckHandle);
		printf("Entering Reset\n");
		Reset(0);
		printf("Returned from reset\n");
		Sender1TimerHandle=xTimerCreate("Sender1Timer", (pdMS_TO_TICKS( Tsender1)),pdTRUE,0,Timer1Callback);
		Sender2TimerHandle=xTimerCreate("Sender2Timer", (pdMS_TO_TICKS( Tsender2)),pdTRUE,0,Timer2Callback);
		ReceiverTimerHandle=xTimerCreate("ReceiverTimer", (pdMS_TO_TICKS( Treceiver)),pdTRUE,0,ReceiverCallback);




	if(  Sender1TimerHandle !=NULL && Sender2TimerHandle!=NULL && ReceiverTimerHandle !=NULL )
	{

		t1=xTimerStart(Sender1TimerHandle,(TickType_t)0);
		t2=xTimerStart(Sender2TimerHandle,(TickType_t)0);
		t3=xTimerStart(ReceiverTimerHandle,(TickType_t)0);
		printf("Timers Active\n");

	}

	if(  t1==1 && t2==1 && t3==1)
	{
		printf("Scheduler Active\n");
		vTaskStartScheduler();
	}

	return 0;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

static void prvOneShotTimerCallback( TimerHandle_t xTimer )
{
	trace_puts("One-shot timer callback executing");
	turn_on (&blinkLeds[1]);
}

static void prvAutoReloadTimerCallback( TimerHandle_t xTimer )
{

	trace_puts("Auto-Reload timer callback executing");

	if(isOn(blinkLeds[0])){
		turn_off(&blinkLeds[0]);
	} else {
		turn_on(&blinkLeds[0]);
	}
}


void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

