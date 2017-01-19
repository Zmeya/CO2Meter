#include <stm32f1xx.h>

#include "MainTask.h"
#include "DisplayTask.h"
#include "fonts.h"

#include "xprintf.h"

CMainTask * CMainTask::m_Instance = 0;

CMainTask::CMainTask()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

	// Конфигурируем CRL регистры. 
	GPIOC->CRH &= ~GPIO_CRH_CNF13;	// Сбрасываем биты CNF для бита 5. Режим 00 - Push-Pull 
	GPIOC->CRH |= GPIO_CRH_MODE13_0;	// Выставляем бит MODE0 для пятого пина. Режим MODE01 = Max Speed 10MHz
}

CMainTask::~CMainTask()
{
}

CMainTask * CMainTask::GetInstance(void)
{
	if (m_Instance == 0)
	{
		m_Instance = new CMainTask();
	}
	return m_Instance;
}

void CMainTask::Run(void const *pParam)
{
	//uint32_t Counter = 0;
	//char Buffer[20];
	uint16_t strWidth = 0, strHeight = 0, strX, strY;
	while (1)
	{
		GPIOC->BSRR = GPIO_BSRR_BR13;
		vTaskDelay(100);
		GPIOC->BSRR = GPIO_BSRR_BS13;
		vTaskDelay(100);
/*		
		char *pWait = "Please Wait...";
		CDisplayTask::GetInstance()->GetStringSize(pWait, &TM_Font_11x18, &strWidth, &strHeight);
		strX = (ILI9341_HEIGHT / 2) - (strWidth / 2);
		strY = (ILI9341_WIDTH / 2); -(strHeight / 2);
		CDisplayTask::GetInstance()->DrawString(strX, strY, pWait, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
*/

	}
}
