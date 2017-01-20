#include <stm32f1xx.h>

#include "MainTask.h"
#include "DisplayDriver.h"
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
	uint16_t strWidth, strHeight, strX, strY;
	char pWait[] = "Please Wait...";

	CDisplayDriver::GetInstance()->GetStringSize(pWait, &TM_Font_11x18, &strWidth, &strHeight);
	strX = (ILI9341_HEIGHT / 2) - (strWidth / 2);
	strY = (ILI9341_WIDTH / 2) - (strHeight / 2);
	CDisplayDriver::GetInstance()->DrawString(strX, strY, pWait, &TM_Font_11x18, Black, White);
	CDisplayDriver::GetInstance()->DrawRectangle(strX - 3, strY - 3, (strX - 3) + (strWidth + 3), (strY - 3) + (strHeight + 3), Black);

	while (1)
	{
		GPIOC->BSRR = GPIO_BSRR_BR13;
		vTaskDelay(100);
		GPIOC->BSRR = GPIO_BSRR_BS13;
		vTaskDelay(100);

	}
}
