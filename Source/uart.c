/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Library includes. */
#include <stm32f1xx.h>

/* Driver includes. */
#include "uart.h"
/*-----------------------------------------------------------*/
/* Queues are used to hold characters that are waiting to be transmitted. This
constant sets the maximum number of characters that can be contained in such a
queue at any one time. */
#define serTX_QUEUE_LEN					( 100 )

/* Queues are used to hold characters that have been received but not yet 
processed.  This constant sets the maximum number of characters that can be 
contained in such a queue. */
#define serRX_QUEUE_LEN					( 100 )

/* The maximum amount of time that calls to lSerialPutString() should wait for
there to be space to post each character to the queue of characters waiting
transmission.  NOTE!  This is the time to wait per character - not the time to
wait for the entire string. */
#define serPUT_STRING_CHAR_DELAY		( 5 / portTICK_PERIOD_MS )

/*-----------------------------------------------------------*/

/* Queues used to hold characters waiting to be transmitted - one queue per port. */
static QueueHandle_t xCharsForTx;

/* Queues holding received characters - one queue per port. */
static QueueHandle_t xRxedChars;

/*-----------------------------------------------------------*/

/* UART interrupt handlers, as named in the vector table. */
void USART1_IRQHandler( void );
void USART2_IRQHandler( void );

/*-----------------------------------------------------------*/

/*
 * See header file for parameter descriptions.
 */
long lCOMPortInit(unsigned long ulWantedBaud )
{
	long lReturn = pdFAIL;

	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN; 	// GPIOA Clock ON. Alter function clock ON
	GPIOA->CRH &= ~GPIO_CRH_CNF9;				// Clear CNF bit 9
	GPIOA->CRH |= GPIO_CRH_CNF9_1;				// Set CNF bit 9 to 10 - AFIO Push-Pull
	GPIOA->CRH |= GPIO_CRH_MODE9_0;				// Set MODE bit 9 to Mode 01 = 10MHz

	GPIOA->CRH &= ~GPIO_CRH_CNF10;				// Clear CNF bit 10
	GPIOA->CRH |= GPIO_CRH_CNF10_0;				// Set CNF bit 10 to 01 = HiZ
	GPIOA->CRH &= ~GPIO_CRH_MODE10;				// Set MODE bit 10 to Mode 01 = 10MHz

	// ¬ключить UART1
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;		// USART1 Clock ON
	USART1->BRR = 128000000 / ulWantedBaud;		// Bodrate for 9600 on 72Mhz( BRR = F_USART / baud)

	USART1->CR1 = 0x00000000;
	USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE; // USART1 ON, TX ON, RX ON

	NVIC_EnableIRQ(USART1_IRQn);
	/* Init the buffer structures with the buffer for the COM port being
	initialised, and perform any non-common initialisation necessary.  This
	does not check to see if the COM port has already been initialised. */
	/* Create the queue of chars that are waiting to be sent to COM0. */
	xCharsForTx = xQueueCreate( serTX_QUEUE_LEN, sizeof( char ) );

	/* Create the queue used to hold characters received from COM0. */
	xRxedChars = xQueueCreate( serRX_QUEUE_LEN, sizeof( char ) );
	
	return lReturn;
}
/*-----------------------------------------------------------*/

signed long xSerialGetChar(char *pcRxedChar, TickType_t xBlockTime )
{
	long lReturn = pdFAIL;

	if (xQueueReceive(xRxedChars, pcRxedChar, xBlockTime) == pdPASS)
	{
		lReturn = pdPASS;
	}

	return lReturn;
}
/*-----------------------------------------------------------*/

long lSerialPutString(char * const pcString, unsigned long ulStringLength )
{
	long lReturn;
	unsigned long ul;

	lReturn = pdPASS;

	for (ul = 0; ul < ulStringLength; ul++)
	{
		if (xQueueSend(xCharsForTx, &(pcString[ul]), serPUT_STRING_CHAR_DELAY) != pdPASS)
		{
			/* Cannot fit any more in the queue.  Try turning the Tx on to clear some space. */
			USART1->CR1 &= ~USART_CR1_UE;
			USART1->CR1 |= USART_CR1_TXEIE;
			USART1->CR1 |= USART_CR1_UE;
			vTaskDelay(serPUT_STRING_CHAR_DELAY);

			/* Go back and try again. */
			continue;
		}
	}

	USART1->CR1 &= ~USART_CR1_UE;
	USART1->CR1 |= USART_CR1_TXEIE;
	USART1->CR1 |= USART_CR1_UE;

	return lReturn;
}
/*-----------------------------------------------------------*/

signed long xSerialPutChar(char cOutChar, TickType_t xBlockTime )
{
	long lReturn;

	if (xQueueSend(xCharsForTx, &cOutChar, xBlockTime) == pdPASS)
	{
		lReturn = pdPASS;
		USART1->CR1 &= ~USART_CR1_UE;
		USART1->CR1 |= USART_CR1_TXEIE;
		USART1->CR1 |= USART_CR1_UE;
	}
	else
	{
		lReturn = pdFAIL;
	}

	return lReturn;
}
/*-----------------------------------------------------------*/

void USART1_IRQHandler1( void )
{
	long xHigherPriorityTaskWoken = pdFALSE;
	char cChar;

	if ((USART1->SR & USART_SR_TXE) == USART_SR_TXE)
	{
		/* The interrupt was caused by the THR becoming empty.  Are there any
		more characters to transmit? */
		if (xQueueReceiveFromISR(xCharsForTx, &cChar, &xHigherPriorityTaskWoken))
		{
			/* A character was retrieved from the buffer so can be sent to the
			THR now. */
			USART1->DR = cChar;
		}
		else
		{
			USART1->CR1 &= ~USART_CR1_UE;
			USART1->CR1 &= USART_CR1_TXEIE;
			USART1->CR1 |= USART_CR1_UE;
		}
	}

	if ((USART1->SR & USART_SR_RXNE) == USART_SR_RXNE)
	{
		cChar = USART1->DR;
		xQueueSendFromISR(xRxedChars, &cChar, &xHigherPriorityTaskWoken);
	}

	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
/*-----------------------------------------------------------*/
