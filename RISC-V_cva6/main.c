/* FreeRTOS kernel includes. */
#include <FreeRTOS.h>
#include "TestRunner.h"
#include <task.h>
#include <uart/uart.h>
static uart_instance_t * const gp_my_uart = &g_uart_0;
static int g_stdio_uart_init_done = 0;

#define mainCREATE_SIMPLE_BLINKY_DEMO_ONLY	0

/*-----------------------------------------------------------*/
#if mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1
	extern void main_blinky( void );
#else
	extern void main_full( void );
#endif 
/*-----------------------------------------------------------*/

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file.  See https://www.freertos.org/a00016.html */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

/* Prepare hardware to run the demo. */
static void prvSetupHardware( void );

/* Send a message to the UART initialised in prvSetupHardware. */
void vSendString( const char * const pcString );

/*-----------------------------------------------------------*/

#define LEDS_ADDR     0x40000000
#define LEDS (*((volatile unsigned int *) (LEDS_ADDR + 0x0)))
static int LED_STATE = 0xf;

uint8_t s_uart[17] = "init_uart SHIP\r\n";

#define testrunnerCHECK_TASK_PRIORITY			( configMAX_PRIORITIES - 2 )
#define testrunnerQUEUE_POLL_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define testrunnerSEM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define testrunnerBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define testrunnerCREATOR_TASK_PRIORITY			( tskIDLE_PRIORITY + 3 )
#define testrunnerFLASH_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define testrunnerINTEGER_TASK_PRIORITY			( tskIDLE_PRIORITY )
#define testrunnerGEN_QUEUE_TASK_PRIORITY		( tskIDLE_PRIORITY )
#define testrunnerFLOP_TASK_PRIORITY			( tskIDLE_PRIORITY )
#define testrunnerQUEUE_OVERWRITE_PRIORITY		( tskIDLE_PRIORITY )
#define testrunnerREGISTER_TEST_PRIORITY		( tskIDLE_PRIORITY )
#define testrunnerTIMER_TEST_PERIOD                             ( 50 )

/*-----------------------------------------------------------*/

int
uart_putc(int c)
{
	int status;

	/* Wait until TX FIFO is empty. */
	do {
		status = *(uint32_t *)(FPGA_UART_0_BASE + 0x14); //gp_my_uart->hw_reg->LSR;
	}
	while (0u == (status & UART_THRE));

	//gp_my_uart->hw_reg->THR = c;
	*(uint32_t *)FPGA_UART_0_BASE = c;

	do {
		//status = gp_my_uart->hw_reg->LSR;
		status = *(uint32_t *)(FPGA_UART_0_BASE + 0x14); //gp_my_uart->hw_reg->LSR;
	}
	while (0u == (status & UART_THRE));
}

int
_write (int file, char * ptr, int len)
{
	int i;

	for (i = 0; i < len; i++)
		uart_putc(ptr[i]);

	return (len);
}

void LEDInit( void )
{
    LEDS = LED_STATE;
}

static void prvSetupHardware( void )
{
    LEDInit();
    
    UART_init(gp_my_uart,
              STDIO_BAUD_RATE,
              UART_DATA_8_BITS | UART_NO_PARITY | UART_ONE_STOP_BIT);
                      
    g_stdio_uart_init_done = 1;
    UART_polled_tx_string(gp_my_uart, s_uart);


	//PLIC_init();
	//UART_init( &g_uart, COREUARTAPB0_BASE_ADDR, BAUD_VALUE_115200, ( DATA_8_BITS | NO_PARITY ) );
}
/*-----------------------------------------------------------*/


void LEDOn( void )
{
    LEDS = 0xf;
}

void LEDOff( void )
{
    LEDS = 0x0;
}

int dummy(int i)
{
    int j = i;
    return j;
}
void vToggleLED( void )
{
    LEDS = LED_STATE;
    LED_STATE = ~LED_STATE;
}

void flash_led()
{   int loop = 5;
    for(int k = 0; k < loop; k++)
    {
        for(int i = 0; i < 1000000;)
        {
          i++;

        }
        vToggleLED();
    }
}

extern void freertos_risc_v_trap_handler( void );

void
my_printf(void *s)
{

	UART_polled_tx_string(gp_my_uart, s);
}

void
test_task(void *arg)
{
	TickType_t xDelay;
	char *s[128];

	int i;

	i = 0;

	xDelay = 500 / portTICK_PERIOD_MS;

	printf("startup1\r\n");
	printf("startup2\r\n");
	printf("startup %d\r\n", xDelay);

	sprintf(s, "%s\r\n", __func__);
	UART_polled_tx_string(gp_my_uart, s);

	xNetworkInterfaceInitialise();
	sprintf(s, "%s: ok\r\n", __func__);
	UART_polled_tx_string(gp_my_uart, s);

	while (1) {
		printf("working %d\r\n", i++);
#if 0
		sprintf(s, "hello from task %d\r\n", i++);
		UART_polled_tx_string(gp_my_uart, s);
#endif
		vTaskDelay(xDelay);
	}
}

#define mainVECTOR_MODE_DIRECT 1

int
main(void)
{
	BaseType_t xResult;

	prvSetupHardware();

	// trap handler initialization
#if (mainVECTOR_MODE_DIRECT == 1)
	{
		__asm__ volatile("csrw mtvec, %0"
			:: "r"(freertos_risc_v_trap_handler));
	}
#else
	{
		__asm__ volatile("csrw mtvec, %0"
			:: "r"((uintptr_t)freertos_vector_table | 0x1));
	}
#endif

	xResult = xTaskCreate(test_task, "test", configMINIMAL_STACK_SIZE * 8U,
	    NULL, 1 /* prio */, NULL );
	if( xResult == pdPASS )
	{
		#if( configSTART_TASK_NOTIFY_TESTS == 1 )
		{
			vStartTaskNotifyTask();
		}
		#endif /* configSTART_TASK_NOTIFY_TESTS */

		#if( configSTART_TASK_NOTIFY_ARRAY_TESTS == 1 )
		{
			vStartTaskNotifyArrayTask();
		}
		#endif /* configSTART_TASK_NOTIFY_ARRAY_TESTS */

		#if( configSTART_BLOCKING_QUEUE_TESTS == 1 )
		{
			vStartBlockingQueueTasks( testrunnerBLOCK_Q_PRIORITY );
		}
		#endif /* configSTART_BLOCKING_QUEUE_TESTS */

		#if( configSTART_SEMAPHORE_TESTS == 1 )
		{
			vStartSemaphoreTasks( testrunnerSEM_TEST_PRIORITY );
		}
		#endif /* configSTART_SEMAPHORE_TESTS */

		#if( configSTART_POLLED_QUEUE_TESTS == 1 )
		{
			vStartPolledQueueTasks( testrunnerQUEUE_POLL_PRIORITY );
		}
		#endif /* configSTART_POLLED_QUEUE_TESTS */

		#if( configSTART_INTEGER_MATH_TESTS == 1 )
		{
			vStartIntegerMathTasks( testrunnerINTEGER_TASK_PRIORITY );
		}
		#endif /* configSTART_INTEGER_MATH_TESTS */

		#if( configSTART_GENERIC_QUEUE_TESTS == 1 )
		{
			vStartGenericQueueTasks( testrunnerGEN_QUEUE_TASK_PRIORITY );
		}
		#endif /* configSTART_GENERIC_QUEUE_TESTS */

		#if( configSTART_PEEK_QUEUE_TESTS == 1 )
		{
			vStartQueuePeekTasks();
		}
		#endif /* configSTART_PEEK_QUEUE_TESTS */

		#if( configSTART_MATH_TESTS == 1 )
		{
			vStartMathTasks( testrunnerFLOP_TASK_PRIORITY );
		}
		#endif /* configSTART_MATH_TESTS */

		#if( configSTART_RECURSIVE_MUTEX_TESTS == 1 )
		{
			vStartRecursiveMutexTasks();
		}
		#endif /* configSTART_RECURSIVE_MUTEX_TESTS */

		#if( configSTART_COUNTING_SEMAPHORE_TESTS == 1 )
		{
			vStartCountingSemaphoreTasks();
		}
		#endif /* configSTART_COUNTING_SEMAPHORE_TESTS */

		#if( configSTART_QUEUE_SET_TESTS == 1 )
		{
			vStartQueueSetTasks();
		}
		#endif /* configSTART_QUEUE_SET_TESTS */

		#if( configSTART_QUEUE_OVERWRITE_TESTS == 1 )
		{
			vStartQueueOverwriteTask( testrunnerQUEUE_OVERWRITE_PRIORITY );
		}
		#endif /* configSTART_QUEUE_OVERWRITE_TESTS */

		#if( configSTART_EVENT_GROUP_TESTS == 1 )
		{
			vStartEventGroupTasks();
		}
		#endif /* configSTART_EVENT_GROUP_TESTS */

		#if( configSTART_INTERRUPT_SEMAPHORE_TESTS == 1 )
		{
			vStartInterruptSemaphoreTasks();
		}
		#endif /* configSTART_INTERRUPT_SEMAPHORE_TESTS */

		#if( configSTART_QUEUE_SET_POLLING_TESTS == 1 )
		{
			vStartQueueSetPollingTask();
		}
		#endif /* configSTART_QUEUE_SET_POLLING_TESTS */

		#if( configSTART_BLOCK_TIME_TESTS == 1 )
		{
			vCreateBlockTimeTasks();
		}
		#endif /* configSTART_BLOCK_TIME_TESTS */

		#if( configSTART_ABORT_DELAY_TESTS == 1 )
		{
			vCreateAbortDelayTasks();
		}
		#endif /* configSTART_ABORT_DELAY_TESTS */

		#if( configSTART_MESSAGE_BUFFER_TESTS == 1 )
		{
			vStartMessageBufferTasks( configMINIMAL_STACK_SIZE );
		}
		#endif /* configSTART_MESSAGE_BUFFER_TESTS */

		#if(configSTART_STREAM_BUFFER_TESTS  == 1 )
		{
			vStartStreamBufferTasks();
		}
		#endif /* configSTART_STREAM_BUFFER_TESTS */

		#if( configSTART_STREAM_BUFFER_INTERRUPT_TESTS == 1 )
		{
			vStartStreamBufferInterruptDemo();
		}
		#endif /* configSTART_STREAM_BUFFER_INTERRUPT_TESTS */

		#if( ( configSTART_TIMER_TESTS == 1 ) && ( configUSE_PREEMPTION != 0 ) )
		{
			/* Don't expect these tasks to pass when preemption is not used. */
			vStartTimerDemoTask( testrunnerTIMER_TEST_PERIOD );
		}
		#endif /* ( configSTART_TIMER_TESTS == 1 ) && ( configUSE_PREEMPTION != 0 ) */

		#if( configSTART_INTERRUPT_QUEUE_TESTS == 1 )
		{
			vStartInterruptQueueTasks();
		}
		#endif /* configSTART_INTERRUPT_QUEUE_TESTS */

		#if( configSTART_REGISTER_TESTS == 1 )
		{
			vStartRegisterTasks( testrunnerREGISTER_TEST_PRIORITY );
		}
		#endif /* configSTART_REGISTER_TESTS */

		#if( configSTART_DELETE_SELF_TESTS == 1 )
		{
			/* The suicide tasks must be created last as they need to know how many
			* tasks were running prior to their creation.  This then allows them to
			* ascertain whether or not the correct/expected number of tasks are
			* running at any given time. */
			vCreateSuicidalTasks( testrunnerCREATOR_TASK_PRIORITY );
		}
		#endif /* configSTART_DELETE_SELF_TESTS */
	}

	vTaskStartScheduler();

	flash_led();

	/* The mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting is described at the top of this file. */

#if (mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1)
	{
		main_blinky();
	}
#else
	{
        // main_full : TestRunner.c
        vStartTests();
	}
#endif
	return 0;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	__asm volatile( "ebreak" );
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;
	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	__asm volatile( "ebreak" );
	for( ;; );
}
/*-----------------------------------------------------------*/


void vAssertCalled( void )
{
	uint8_t s_debug[16] = "vassert called\r\n";

	portENTER_CRITICAL();
	UART_polled_tx_string(gp_my_uart, s_debug);
	portEXIT_CRITICAL();

	volatile uint32_t ulSetTo1ToExitFunction = 0;

	taskDISABLE_INTERRUPTS();

	while (ulSetTo1ToExitFunction != 1)
		__asm volatile("NOP");
}
