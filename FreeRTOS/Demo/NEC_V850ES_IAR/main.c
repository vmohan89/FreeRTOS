/*
    FreeRTOS V7.4.2 - Copyright (C) 2013 Real Time Engineers Ltd.

    FEATURES AND PORTS ARE ADDED TO FREERTOS ALL THE TIME.  PLEASE VISIT
    http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.

    >>>>>>NOTE<<<<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License
    and the FreeRTOS license exception along with FreeRTOS; if not it can be
    viewed here: http://www.freertos.org/a00114.html and also obtained by
    writing to Real Time Engineers Ltd., contact details for whom are available
    on the FreeRTOS WEB site.

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************


    http://www.FreeRTOS.org - Documentation, books, training, latest versions, 
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, and our new
    fully thread aware and reentrant UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High 
    Integrity Systems, who sell the code with commercial support, 
    indemnification and middleware, under the OpenRTOS brand.
    
    http://www.SafeRTOS.com - High Integrity Systems also provide a safety 
    engineered and independently SIL3 certified version for use in safety and 
    mission critical applications that require provable dependability.
*/

/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the standard demo application tasks.
 * In addition to the standard demo tasks, the following tasks and tests are
 * defined and/or created within this file:
 *
 * "Check" task -  This only executes every three seconds but has a high priority
 * to ensure it gets processor time.  Its main function is to check that all the
 * standard demo tasks are still operational.  If everything is running as
 * expected then the check task will toggle an LED every 3 seconds.  An error
 * being discovered in any task will cause the toggle rate to increase to 500ms.
 *
 * "Reg test" tasks - These fill the registers with known values, then check
 * that each register still contains its expected value.  Each task uses
 * different values.  The tasks run with very low priority so get preempted very
 * frequently.  A register containing an unexpected value is indicative of an
 * error in the context switching mechanism.
 *
 */

/* Standard include files. */
#include <stdlib.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo file headers. */
#include <intrinsics.h>
#include "BlockQ.h"
#include "death.h"
#include "flash.h"
#include "partest.h"
#include "semtest.h"
#include "PollQ.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "recmutex.h"
#include "comtest2.h"

/*
 * Priority definitions for most of the tasks in the demo application.  Some
 * tasks just use the idle priority.
 */
#define mainFLASH_PRIORITY					( tskIDLE_PRIORITY + 1 )
#define mainQUEUE_POLL_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCHECK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )
#define mainSEM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCREATOR_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2 )
#define mainINTEGER_TASK_PRIORITY           ( tskIDLE_PRIORITY )
#define mainGEN_QUEUE_TASK_PRIORITY			( tskIDLE_PRIORITY )
#define mainCOMTEST_PRIORITY				( tskIDLE_PRIORITY + 1 )

/* Passed into the check task just as a test that the parameter passing
mechanism is working correctly. */
#define mainCHECK_PARAMETER					( ( void * ) 0x12345678 )

/* The period between executions of the check task. */
#define mainNO_ERROR_DELAY		( ( portTickType ) 3000 / portTICK_RATE_MS  )
#define mainERROR_DELAY			( ( portTickType ) 500 / portTICK_RATE_MS )

/* There are no spare LEDs for the comtest tasks, so this is just set to an
invalid number. */
#define mainCOMTEST_LED			( 4 )

/* The baud rate used by the comtest task. */
#define mainBAUD_RATE			( 9600 )

/*-----------------------------------------------------------*/

/* The implementation of the 'check' task as described at the top of this file. */
static void prvCheckTask( void *pvParameters );

/* Just sets up the LED outputs.  Most generic setup is done in
__low_level_init(). */
static void prvSetupHardware( void );

/* The RegTest functions as described at the top of this file. */
extern void vRegTest1( void *pvParameters );
extern void vRegTest2( void *pvParameters );

/* A variable that will get set to fail if a RegTest task finds an error.  The
variable is inspected by the 'Check' task. */
static volatile portLONG lRegTestStatus = pdPASS;

/*-----------------------------------------------------------*/

/* Create all the demo tasks then start the scheduler. */
void main( void )
{
	/* Just sets up the LED outputs. */
	prvSetupHardware();

	/* Standard demo tasks. */
	vStartSemaphoreTasks( mainSEM_TEST_PRIORITY );
	vStartGenericQueueTasks( mainGEN_QUEUE_TASK_PRIORITY );
	vStartQueuePeekTasks();
	
	/* Create the check task as described at the top of this file. */
	xTaskCreate( prvCheckTask, "Check", configMINIMAL_STACK_SIZE, mainCHECK_PARAMETER, mainCHECK_TASK_PRIORITY, NULL );

	/* Create the RegTest tasks as described at the top of this file. */
	xTaskCreate( vRegTest1, "Reg1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
	xTaskCreate( vRegTest2, "Reg2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );

	#ifdef __IAR_V850ES_Fx3__
	{
		/* The extra IO required for the com test and led flashing tasks is only
		available on the application board, not the target boards. */	
		vAltStartComTestTasks( mainCOMTEST_PRIORITY, mainBAUD_RATE, mainCOMTEST_LED );
		vStartLEDFlashTasks( mainFLASH_PRIORITY );
		
		/* The Fx3 also has enough RAM to run loads more tasks. */
		vStartRecursiveMutexTasks();
		vStartBlockingQueueTasks( mainBLOCK_Q_PRIORITY );
		vStartPolledQueueTasks( mainQUEUE_POLL_PRIORITY );				
	}
	#endif	
	
	/* The suicide tasks must be created last as they need to know how many
	tasks were running prior to their creation in order to ascertain whether
	or not the correct/expected number of tasks are running at any given time. */
    vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	/* If this line is reached then vTaskStartScheduler() returned because there
	was insufficient heap memory remaining for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvCheckTask( void *pvParameters )
{
portTickType xDelayPeriod = mainNO_ERROR_DELAY, xLastWakeTime;
unsigned portBASE_TYPE uxLEDToUse = 0;

	/* Ensure parameter is passed in correctly. */
	if( pvParameters != mainCHECK_PARAMETER )
	{
		xDelayPeriod = mainERROR_DELAY;
	}
	
	/* Initialise xLastWakeTime before it is used.  After this point it is not
	written to directly. */
	xLastWakeTime = xTaskGetTickCount();
	
	/* Cycle for ever, delaying then checking all the other tasks are still
	operating without error. */
	for( ;; )
	{
		/* Wait until it is time to check all the other tasks again. */
		vTaskDelayUntil( &xLastWakeTime, xDelayPeriod );
		
		if( lRegTestStatus != pdPASS )
		{
			xDelayPeriod = mainERROR_DELAY;
		}
		
		if( xAreGenericQueueTasksStillRunning() != pdTRUE )
		{
			xDelayPeriod = mainERROR_DELAY;
		}

		if( xAreQueuePeekTasksStillRunning() != pdTRUE )
		{
			xDelayPeriod = mainERROR_DELAY;
		}

		if( xAreSemaphoreTasksStillRunning() != pdTRUE )
	    {
	    	xDelayPeriod = mainERROR_DELAY;
	    }

		if( xIsCreateTaskStillRunning() != pdTRUE )
	    {
	    	xDelayPeriod = mainERROR_DELAY;
	    }

		/* The Fx3 runs more tasks, so more checks are performed. */		
		#ifdef __IAR_V850ES_Fx3__
		{
			if( xAreComTestTasksStillRunning() != pdTRUE )
			{
				xDelayPeriod = mainERROR_DELAY;
			}
			
			if( xArePollingQueuesStillRunning() != pdTRUE )
			{
				xDelayPeriod = mainERROR_DELAY;
			}

			if( xAreBlockingQueuesStillRunning() != pdTRUE )
			{
				xDelayPeriod = mainERROR_DELAY;
			}
			
			if( xAreRecursiveMutexTasksStillRunning() != pdTRUE )
			{
				xDelayPeriod = mainERROR_DELAY;
			}		
			
			/* The application board has more LEDs and uses the flash tasks
			so the check task instead uses LED3 as LED3 is still spare. */
			uxLEDToUse = 3;
		}
		#endif

		/* Toggle the LED.  The toggle rate will depend on whether or not an
		error has been found in any tasks. */
		vParTestToggleLED( uxLEDToUse );
	}
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Setup the LED outputs. */
	vParTestInitialise();

	/* Any additional hardware configuration can be added here. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( void )
{
	/* This will be called if a task overflows its stack.  pxCurrentTCB
	can be inspected to see which is the offending task. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vRegTestFailed( void )
{
	/* Called by the RegTest tasks if an error is found.  lRegTestStatus is
	inspected by the check task. */
	lRegTestStatus = pdFAIL;
	
	/* Do not return from here as the reg test tasks clobber all registers so
	function calls may not function correctly. */
	for( ;; );
}
