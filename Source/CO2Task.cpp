#include "FreeRTOS.h"
#include "stm32f1xx.h"
#include "semphr.h"
#include "queue.h"
#include "CO2Task.h"
#include "uart.h"
#include "xprintf.h"
#include "DisplayTask.h"
#include "fonts.h"

xSemaphoreHandle gUARTSemaphore;
xQueueHandle gCO2Data;

/*
0xFF Ч начало любой команды
0x01 Ч первый сенсор(он всего один)
0x86 Ч команда
0x00, 0x00, 0x00, 0x00, 0x00 Ч данные
0x79 Ч контрольна€ сумма.
*/

const uint8_t CO2Cmd[9] = { 0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79 };
uint8_t CO2Reseive[9];

typedef struct CO2Data
{
	uint8_t bStartingByte;
	uint8_t bCommand;
	uint8_t bHLConcentration; // Gas concentration= high level * 256 + low level
	uint8_t bLLConcentration;
	uint8_t bReserved[4];
	uint8_t bCRC;
} CO2Data_t;

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
	USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; // USART1 ON, TX ON, RX ON
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
	char Recv = 0;
	uint16_t strWidth = 0, strHeight = 0, strX, strY;
	//vTaskDelay(1000);
	//CDisplayTask::GetInstance()->DrawString(0, 0, "Please Wait...", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	//vTaskDelay(25000);

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

		do
		{
			while ((USART1->SR & USART_SR_RXNE) != USART_SR_RXNE);
			Recv = USART1->DR;

		} while (Recv != 0xFF);
		
		taskENTER_CRITICAL();
		for(int i = 1; i <= 8; i++)
		{
			while ((USART1->SR & USART_SR_RXNE) != USART_SR_RXNE);
			CO2Reseive[i] = USART1->DR;
		}
		taskEXIT_CRITICAL();

		CO2Reseive[0] = Recv;

		char PackCRC = 0;
		for (int i = 1; i < 8; i++) {
			PackCRC += CO2Reseive[i];
		}
		PackCRC = 255 - PackCRC;
		PackCRC++;

		if (CO2Reseive[0] == 0xFF && CO2Reseive[1] == 0x86 && CO2Reseive[8] == PackCRC) {
			int co2 = CO2Reseive[2] * 255 + CO2Reseive[3];
			xsprintf(pBuffer, "CO2 = %04d ppm", co2);
			CDisplayTask::GetInstance()->GetStringSize(pBuffer, &TM_Font_16x26, &strWidth, &strHeight);
			strX = (ILI9341_HEIGHT / 2) - (strWidth / 2);
			strY = (ILI9341_WIDTH / 2) - (strHeight / 2);
			CDisplayTask::GetInstance()->DrawString(strX, strY, pBuffer, &TM_Font_16x26, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
			CDisplayTask::GetInstance()->DrawRectangle(strX - 3, strY - 3, (strX - 3) + (strWidth + 3) + 1, (strY - 3) + (strHeight + 3) + 1, ILI9341_COLOR_BLACK);
		}
		else
		{
			char *pWait = "Please Wait...";
			CDisplayTask::GetInstance()->GetStringSize(pWait, &TM_Font_11x18, &strWidth, &strHeight);
			strX = (ILI9341_HEIGHT / 2) - (strWidth / 2);
			strY = (ILI9341_WIDTH / 2) - (strHeight / 2);
			CDisplayTask::GetInstance()->DrawString(strX, strY, pWait, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
			CDisplayTask::GetInstance()->DrawRectangle(strX - 3, strY - 3, (strX - 3) + (strWidth + 3), (strY - 3) + (strHeight + 3), ILI9341_COLOR_BLACK);
		}

		vTaskDelay(5000);
	}
}