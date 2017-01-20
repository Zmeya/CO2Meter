#include "FreeRTOS.h"
#include "stm32f1xx.h"
#include "semphr.h"
#include "queue.h"
#include "CO2Task.h"
#include "uart.h"
#include "xprintf.h"
#include "DisplayDriver.h"
#include "fonts.h"

extern TaskHandle_t CO2Task;

/*
0xFF Ч начало любой команды
0x01 Ч первый сенсор(он всего один)
0x86 Ч команда
0x00, 0x00, 0x00, 0x00, 0x00 Ч данные
0x79 Ч контрольна€ сумма.
*/

const uint8_t CO2Cmd[9] = { 0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79 };
volatile uint8_t CO2Reseive[9] = { 0 };
uint16_t CO2BarGraph[ILI9341_HEIGHT] = { 400, 500, 800, 2000, 5000, 4000, 300, 1200, 1500, 40 };

CCO2Task * CCO2Task::m_Instance = 0;

CCO2Task::CCO2Task()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN; 	// GPIOA Clock ON. Alter function clock ON
	GPIOA->CRH &= ~GPIO_CRH_CNF9;				// Clear CNF bit 9
	GPIOA->CRH |= GPIO_CRH_CNF9_1;				// Set CNF bit 9 to 10 - AFIO Push-Pull
	GPIOA->CRH |= GPIO_CRH_MODE9_0;				// Set MODE bit 9 to Mode 01 = 10MHz

	GPIOA->CRH &= ~GPIO_CRH_CNF10;				// Clear CNF bit 10
	GPIOA->CRH |= GPIO_CRH_CNF10_0;				// Set CNF bit 10 to 01 = HiZ
	GPIOA->CRH &= ~GPIO_CRH_MODE10;				// Set MODE bit 10 to Mode 01 = 10MHz

												// ¬ключить UART1
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;		// USART1 Clock ON
	USART1->BRR = SystemCoreClock / 9600;		// Bodrate for 9600 on 72Mhz( BRR = F_USART / baud)

	USART1->CR1 = 0x00000000;
	USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE; // USART1 ON, TX ON, RX ON
	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_SetPriority(USART1_IRQn, 15);
}

CCO2Task::~CCO2Task()
{
}

CCO2Task * CCO2Task::GetInstance(void)
{
	if (m_Instance == 0)
	{
		m_Instance = new CCO2Task();
	}
	return m_Instance;
}

void CCO2Task::Run(void const *pParam)
{
	uint16_t strWidth = 0, strHeight = 0, strX, strY, nCount = 0, CurPos = 0;

	char pBuffer[20] = { 0 };
	while (1)
	{
		taskENTER_CRITICAL();
		for (int i = 0; i < 9; i++)
		{
			USART1->DR = CO2Cmd[i];
			while ((USART1->SR & USART_SR_TC) != USART_SR_TC);
		}
		taskEXIT_CRITICAL();

		ulTaskNotifyTake(pdTRUE, 1000);
		
		char PackCRC = 0;
		for (int i = 1; i < 8; i++) {
			PackCRC += CO2Reseive[i];
		}
		PackCRC = 255 - PackCRC;
		PackCRC++;

		if (CO2Reseive[0] == 0xFF && CO2Reseive[1] == 0x86 && CO2Reseive[8] == PackCRC) {
			int co2 = CO2Reseive[2] * 255 + CO2Reseive[3];
			xsprintf(pBuffer, "CO2 = %04d ppm", co2);
			CDisplayDriver::GetInstance()->GetStringSize(pBuffer, &TM_Font_16x26, &strWidth, &strHeight);
			strX = (ILI9341_HEIGHT / 2) - (strWidth / 2);
			strY = (ILI9341_WIDTH / 2) - (strHeight / 2);
			CDisplayDriver::GetInstance()->DrawString(strX, strY, pBuffer, &TM_Font_16x26, Black, White);
			CDisplayDriver::GetInstance()->DrawRectangle(strX - 3, strY - 3, (strX - 3) + (strWidth + 4), (strY - 3) + (strHeight + 4), Black);
			memset((void*)CO2Reseive, 0, 9);

			CO2BarGraph[nCount++] = co2;
			if (CO2BarGraph[nCount-1] > 2500)
				CO2BarGraph[nCount-1] = 2500;
			if (nCount > ILI9341_HEIGHT - 2)
			{
				nCount = 0;
				
			}
			CurPos = 0;
			float clr = strY + strHeight + 10;
			CDisplayDriver::GetInstance()->DrawLine(0, clr - 1, ILI9341_HEIGHT, clr - 1, Black);
			CDisplayDriver::GetInstance()->FillRectangle(0, clr, ILI9341_HEIGHT, ILI9341_WIDTH, White);
			for (uint16_t i = ILI9341_HEIGHT - nCount; i < ILI9341_HEIGHT; i++, CurPos++)
				CDisplayDriver::GetInstance()->DrawLine(i, ILI9341_WIDTH - (CO2BarGraph[CurPos] * (clr) / 3700.0), i, ILI9341_WIDTH, CO2BarGraph[CurPos] >= 2000 ? Red : Green);
		}

		vTaskDelay(5000);
	}
}

extern "C" void USART1_IRQHandler(void)
{
	long xHigherPriorityTaskWoken = pdFALSE;
	if ((USART1->SR & USART_SR_RXNE) == USART_SR_RXNE)
	{
		if(USART1->DR == 0xFF)
		{
			CO2Reseive[0] = 0xFF;
			NVIC_DisableIRQ(USART1_IRQn);
			for (int i = 1; i <= 8; i++)
			{
				while ((USART1->SR & USART_SR_RXNE) != USART_SR_RXNE);
				CO2Reseive[i] = USART1->DR;
				if (CO2Reseive[1] != 0x86)
				{
					NVIC_EnableIRQ(USART1_IRQn);
					return;
				}
			}
			vTaskNotifyGiveFromISR(CO2Task, &xHigherPriorityTaskWoken);
			NVIC_EnableIRQ(USART1_IRQn);
		}
	}
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	return;
}