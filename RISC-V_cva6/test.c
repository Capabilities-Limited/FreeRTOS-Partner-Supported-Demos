/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_UDP_IP.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_DNS.h"
#include "NetworkBufferManagement.h"

const char *
pcApplicationHostnameHook(void)
{

	return "cva6";
}

BaseType_t
xApplicationGetRandomNumber( uint32_t * pulNumber )
{

	*pulNumber = 5;

	return pdTRUE;
}

void
vApplicationIPNetworkEventHook(eIPCallbackEvent_t eNetworkEvent)
{

	printf("aaaaaa\n");
}

struct xNetworkEndPoint * my_pxEndPoint;

void
vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent,
    struct xNetworkEndPoint * pxEndPoint)
{
	static BaseType_t xTasksAlreadyCreated = pdFALSE;

	if (eNetworkEvent == eNetworkUp) {
		if (xTasksAlreadyCreated == pdFALSE) {
			/* Create the tasks here. */
			xTasksAlreadyCreated = pdTRUE;
		}
		//showEndPoint(pxEndPoint);
	}
	my_pxEndPoint = pxEndPoint;
}

uint32_t
ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress,
    uint16_t usSourcePort, uint32_t ulDestinationAddress,
    uint16_t usDestinationPort)
{

}
