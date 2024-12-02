/*
FreeRTOS+TCP V2.0.11
Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 http://aws.amazon.com/freertos
 http://www.FreeRTOS.org
*/

/* Standard includes. */
#include <stdint.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "FreeRTOS_IP.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_UDP_IP.h"
#include "FreeRTOS_Sockets.h"
#include "NetworkBufferManagement.h"

/* FreeRTOS+TCP includes. */
#include "NetworkInterface.h"

/* Board specific includes */
#include "riscv_hal_eth.h"

#include "plic/plic.h"

#define	ROUNDUP(a, b)	((((a)-1)/(b)+1)*(b))

#define	FIFO_BASE	0x30000000
#define	FIFO_ISR	(FIFO_BASE + 0x00) /* Interrupt Status Register */
#define	FIFO_IER	(FIFO_BASE + 0x04) /* Interrupt Enable Register */
#define	 ISR_RRC	(1 << 23) /* Receive Reset Complete */
#define	 ISR_TRC	(1 << 24) /* Transmit Reset Complete */
#define	 ISR_RC		(1 << 26) /* Receive Complete */
#define	FIFO_TDFR	(FIFO_BASE + 0x08) /* Transmit Data FIFO Reset */
#define	FIFO_TDFD	(FIFO_BASE + 0x10)
#define	FIFO_TLR	(FIFO_BASE + 0x14)
#define	FIFO_RDFR	(FIFO_BASE + 0x18) /* Receive Data FIFO reset */
#define	FIFO_TDFV	(FIFO_BASE + 0x0C) /* Transmit Data FIFO Vacancy */
#define	FIFO_RDFD	(FIFO_BASE + 0x20) /* Receive Data FIFO Data Read Port*/
#define	FIFO_RLR	(FIFO_BASE + 0x24) /* Receive Length Register */
#define	FIFO_TDR	(FIFO_BASE + 0x2C) /* Transmit Destination Register */
#define	FIFO_RDR	(FIFO_BASE + 0x30) /* Receive Destination Register */

static uint32_t aligned_buf[1024];
static uint32_t aligned_rxbuf[1024];
static NetworkInterface_t *my_pxInterface;
static TaskHandle_t rxtask_handle = NULL;

extern struct xNetworkEndPoint * my_pxEndPoint;

// Netboot needs this
BaseType_t xNetworkInterfaceDestroy(void);

void
rxtask(void *arg)
{
	NetworkBufferDescriptor_t *pxBufferDescriptor;
	IPStackEvent_t xRxEvent;
	uint8_t buf[128];
	int len;
	int cnt;
	int i;

	for (;;) {
		taskENTER_CRITICAL();
		len = *(volatile uint32_t *)FIFO_RLR;

		uint32_t ulIPAddress;
		ulIPAddress = FreeRTOS_GetIPAddress();
		FreeRTOS_inet_ntoa(ulIPAddress, buf);
		printf("%s: My IP Address: %s\r\n", __func__, buf);

		if (len == 0) {
			/* wait for notification */
			taskEXIT_CRITICAL();
			ulTaskNotifyTake( pdFALSE, portMAX_DELAY );
			continue;
		}

		printf("%s: received %d bytes\r\n", __func__, len); //* 4);

		pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(len, 0);
		//pxBufferDescriptor = pxNetworkBufferGetFromISR(len);
		printf("pxBufferDescriptor %p\r\n", pxBufferDescriptor);

		if (pxBufferDescriptor == NULL)
			while (1)
				printf("error\r\n");

		pxBufferDescriptor->xDataLength = len; //* 4;
		cnt = ROUNDUP(len, 4);
		cnt /= 4;
		printf("%s: len %d cnt %d\r\n", __func__, len, cnt);
		for (i = 0; i < cnt; i++)
			aligned_rxbuf[i] = *(volatile uint32_t *)FIFO_RDFD;
		taskEXIT_CRITICAL();

		memcpy(pxBufferDescriptor->pucEthernetBuffer, aligned_rxbuf,
		    pxBufferDescriptor->xDataLength);

		pxBufferDescriptor->pxInterface = my_pxInterface;
		pxBufferDescriptor->pxEndPoint =
		    FreeRTOS_FirstEndPoint(my_pxInterface);

		if (eConsiderFrameForProcessing(
		    pxBufferDescriptor->pucEthernetBuffer) == eProcessBuffer) {
			xRxEvent.eEventType = eNetworkRxEvent;
			xRxEvent.pvData = (void *)pxBufferDescriptor;
			if (xSendEventStructToIPTask(&xRxEvent, 0) == pdFALSE) {
				vReleaseNetworkBufferAndDescriptor(
				    pxBufferDescriptor);
				iptraceETHERNET_RX_EVENT_LOST();
			} else {
				printf("PACKED RECEIVED\r\n");
				iptraceNETWORK_INTERFACE_RECEIVE();
			}
		} else
			vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);

		printf("%s: done\r\n", __func__);
	}
}

BaseType_t
xNetworkInterfaceInitialise(void)
{
	uint32_t reg;

	FreeRTOS_debug_printf( ("xNetworkInterfaceInitialise\r\n") );

	/* Reset FIFOs. */
	*(volatile uint32_t *)FIFO_TDFR = 0xa5;
	*(volatile uint32_t *)FIFO_RDFR = 0xa5;

	do {
		reg = *(volatile uint32_t *)FIFO_ISR;
		if ((reg & (ISR_TRC | ISR_RRC)) == (ISR_TRC | ISR_RRC))
			break;
	} while (1);

	xTaskCreate(rxtask, "txtask", configMINIMAL_STACK_SIZE * 5, NULL,
	    ipconfigIP_TASK_PRIORITY - 1, &rxtask_handle);

	/* Init DMA */
	//configASSERT(DmaSetup(&AxiDmaInstance, XPAR_AXIDMA_0_DEVICE_ID) == 0);

	printf("%s\r\n", __func__);

	/* Init Ethernet */
	configASSERT(PhySetup(&AxiEthernetInstance,
	    XPAR_AXIETHERNET_0_DEVICE_ID) == 0);

#if 0
	/* Connect to PLIC */
	configASSERT(AxiEthernetSetupIntrSystem(&Plic, &AxiEthernetInstance,
	    &AxiDmaInstance, PLIC_SOURCE_ETH, XPAR_AXIETHERNET_0_DMA_RX_INTR,
	    XPAR_AXIETHERNET_0_DMA_TX_INTR) == 0);
#endif

#if 0
	uint16_t Speed;
	int dpx;
	int dpx2;
	configASSERT( XAxiEthernet_GetRgmiiStatus(&AxiEthernetInstance, &Speed,
	    &dpx, &dpx2) == 0);
	printf("hello Speed %d dpx %d dpx2 %d\r\n", Speed, dpx, dpx2);
#endif

	/*
	 * Start the Axi Ethernet and enable its ERROR interrupts
	 */
	XAxiEthernet_Start(&AxiEthernetInstance);
	XAxiEthernet_IntEnable(&AxiEthernetInstance, XAE_INT_RECV_ERROR_MASK);

	return pdPASS;
}
/*-----------------------------------------------------------*/

BaseType_t
xNetworkInterfaceDestroy(void)
{
	FreeRTOS_debug_printf( ("xNetworkInterfaceDestroy\r\n") );
	XAxiEthernet_IntDisable(&AxiEthernetInstance, XAE_INT_RECV_ERROR_MASK);
#if 0
	AxiEthernetDisableIntrSystem(&Plic, PLIC_SOURCE_ETH,
	    XPAR_AXIETHERNET_0_DMA_RX_INTR, XPAR_AXIETHERNET_0_DMA_TX_INTR);
#endif
	XAxiDma_Reset(&AxiDmaInstance);
	XAxiEthernet_Reset(&AxiEthernetInstance);
	PhyReset(&AxiEthernetInstance);
	return pdPASS;
}

/*-----------------------------------------------------------*/

void * local_memcpy(void* dest, void* src, size_t size);

BaseType_t
xNetworkInterfaceOutput(NetworkBufferDescriptor_t * const pxNetworkBuffer,
    BaseType_t xReleaseAfterSend)
{
	/* get BD ring descriptor */
	XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AxiDmaInstance);
	XAxiDma_Bd * BdPtr;

	FreeRTOS_debug_printf( ("xNetworkInterfaceOutput\r\n") );

	/* allocate next BD from the BD ring */
	taskENTER_CRITICAL();
	configASSERT( XAxiDma_BdRingAlloc(TxRingPtr, 1, &BdPtr) == 0);
	taskEXIT_CRITICAL();

	/* configure BD */
	uint8_t* xTxBuffer = AxiEthernetGetTxBuffer();

#if (configCHERI_INT_MEMCPY == 1)
	local_memcpy(xTxBuffer, pxNetworkBuffer->pucEthernetBuffer,
	    pxNetworkBuffer->xDataLength);
#else
	memcpy(xTxBuffer, pxNetworkBuffer->pucEthernetBuffer,
	    pxNetworkBuffer->xDataLength);
#endif

#if defined(__clang__)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#endif
	XAxiDma_BdSetBufAddr(BdPtr,(u32)xTxBuffer);
	XAxiDma_BdSetLength(BdPtr, pxNetworkBuffer->xDataLength,
	    TxRingPtr->MaxTransferLen);
	XAxiDma_BdSetCtrl(BdPtr, XAXIDMA_BD_CTRL_TXSOF_MASK |
			     XAXIDMA_BD_CTRL_TXEOF_MASK);
#if defined(__clang__)
#else
#pragma GCC diagnostic pop
#endif	
	/* pass BD to HW */
	taskENTER_CRITICAL();
	configASSERT( XAxiDma_BdRingToHw(TxRingPtr, 1, BdPtr) == 0 );

	/* start transaction */
	configASSERT( XAxiDma_BdRingStart(TxRingPtr) == 0);
	taskEXIT_CRITICAL();

	/* Call the standard trace macro to log the send event. */
	iptraceNETWORK_INTERFACE_TRANSMIT();

	// simple driver
	if (xReleaseAfterSend != pdFALSE ) {
		/*
		 * It is assumed SendData() copies the data out of the
		 * FreeRTOS+TCP Ethernet buffer.  The Ethernet buffer is
		 * therefore no longer needed, and must be freed for re-use.
		 */
		vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
	}

	return pdPASS;
}
/*-----------------------------------------------------------*/

BaseType_t
xNetworkInterfaceOutput_Fifo(NetworkInterface_t * pxInterface,
    NetworkBufferDescriptor_t * const pxNetworkBuffer,
    BaseType_t xReleaseAfterSend)
{
	uint8_t *buf;
	size_t len;
	int cnt;
	int i;

printf("%s\r\n", __func__);

	portENTER_CRITICAL();

	len = pxNetworkBuffer->xDataLength;
	buf = pxNetworkBuffer->pucEthernetBuffer;

	memcpy(aligned_buf, pxNetworkBuffer->pucEthernetBuffer, len);

	for (i = 7; i < 9; i++)
		PLIC_EnableIRQ(i);

	*(volatile uint32_t *)FIFO_TDR = 0;
	*(volatile uint32_t *)FIFO_IER = ISR_RC;

	printf("%s: status0 %x\r\n", __func__, *(uint32_t *)FIFO_ISR);

	cnt = ROUNDUP(len, 4);
	printf("cnt %d len %d\r\n", cnt, len);
	cnt /= 4;

	printf("%s: sending %d bytes\r\n", __func__, len);
	for (i = 0; i < cnt; i += 1)
		*(volatile uint32_t *)FIFO_TDFD = aligned_buf[i];

	//printf("len %d vac %d\r\n", len, *(uint32_t *)FIFO_TDFV);

	printf("%s: sending %d bytes DONE\r\n", __func__, len);

	//printf("%s: status1 %x\r\n", __func__, *(uint32_t *)FIFO_ISR);

	*(volatile uint32_t *)FIFO_TLR = len;

	//printf("%s: status2 %x\r\n", __func__, *(uint32_t *)FIFO_ISR);

	if (xReleaseAfterSend != pdFALSE)
		vReleaseNetworkBufferAndDescriptor(pxNetworkBuffer);

	portEXIT_CRITICAL();

	return (pdPASS);
}

BaseType_t
xNetworkInterfaceInput(void)
{

	static BaseType_t askForContextSwitch = pdFALSE;
	*(volatile uint32_t *)FIFO_ISR = (1 << 26);
	vTaskNotifyGiveFromISR(rxtask_handle, &askForContextSwitch);

	return (0);
}

/* --------- */

BaseType_t
xGetPhyLinkStatus(struct xNetworkInterface *net)
{

	return pdPASS;

	if (PhyLinkStatus(&AxiEthernetInstance))
		printf("%s: link is UP\r\n", __func__);
	else
		printf("%s: link is DOWN\r\n", __func__);

	if (PhyLinkStatus(&AxiEthernetInstance))
		return pdPASS;
	else
		return pdFAIL;
}
/*-----------------------------------------------------------*/

NetworkInterface_t *
pxAxi_FillInterfaceDescriptor(BaseType_t xEMACIndex,
    NetworkInterface_t * pxInterface)
{
	static char pcName[17];
	snprintf(pcName, sizeof( pcName ), "eth%u", ( unsigned ) xEMACIndex);

	memset( pxInterface, '\0', sizeof( *pxInterface ) );

	pxInterface->pcName = pcName;
	pxInterface->pvArgument = ( void * ) xEMACIndex;
	pxInterface->pfInitialise = xNetworkInterfaceInitialise;
	pxInterface->pfOutput = xNetworkInterfaceOutput_Fifo;
	pxInterface->pfGetPhyLinkStatus = xGetPhyLinkStatus;
	FreeRTOS_AddNetworkInterface( pxInterface );

	my_pxInterface = pxInterface;
	return pxInterface;
}
