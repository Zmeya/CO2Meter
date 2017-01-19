#include "stm32f1xx.h"
#include "cmsis_os.h"
#include "task.h"
#include "MainTask.h"
#include "CO2Task.h"
#include "DisplayTask.h"

void TaskRun(void *pParam);

#define HSEStartUp_TimeOut 1000
int RCC_Init(void)
{
	__IO uint32_t StartUpCounter = 0, HSEStatus = 0;

	// ������������  SYSCLK, HCLK, PCLK2 � PCLK1
	RCC->CR |= RCC_CR_HSEON;	// �������� HSE

	// ���� ���� HSE �� �������� ��� ���������� ���� �� ������ �������
	do
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	} while ((HSEStatus == 0) && (StartUpCounter != HSEStartUp_TimeOut));

	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
		HSEStatus = (uint32_t)0x01;
	else
		HSEStatus = (uint32_t)0x00;

	// ���� HSE �� ���������� ���������
	if (HSEStatus != (uint32_t)0x01)
		return -1;

	FLASH->ACR |= FLASH_ACR_PRFTBE;// �������� ����� ����������� FLASH

	// ������������� Flash �� 2 ����� ��������
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |= FLASH_ACR_LATENCY_2;

	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;	// HCLK = SYSCLK

	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;	// PCLK2 = HCLK

	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;	// PCLK1 = HCLK

	// ������������� ��������� PLL configuration: PLLCLK = HSE * 9 = 72 MHz (HSE * 16 = 128 MHz)
	RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
	RCC->CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL16;		// PLL MULL = 16

	RCC->CR |= RCC_CR_PLLON;	// �������� PLL

	while ((RCC->CR & RCC_CR_PLLRDY) == 0);	// �������, ���� PLL �������� ��� ����������

	// �������� PLL ��� �������� ��������� �������
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	while ((RCC->CFGR & RCC_CFGR_SWS) != 0x08);	//�������, ���� PLL ��������� ��� �������� ��������� �������

	return 0;
}

int main(void)
{
	if (RCC_Init() == -1)
		SystemInit();
	SystemCoreClockUpdate();

	xTaskCreate(TaskRun, "MainTask", 80, CMainTask::GetInstance(), 0, NULL);
	xTaskCreate(TaskRun, "CO2Task", 240, CCO2Task::GetInstance(), 0, NULL);
	xTaskCreate(TaskRun, "DisplayTask", 50, CDisplayTask::GetInstance(), 0, NULL);

	osKernelStart();
	while (1);
}
void TaskRun(void *pParam)
{
	((iTask*)pParam)->Run(NULL);
	while (1);
}



void* operator new(size_t sz)
{
	return pvPortMalloc(sz);
}
void* operator new[](size_t sz)
{
	return pvPortMalloc(sz);
}
void operator delete(void * p)
{
	vPortFree(p);
}
void operator delete[](void * p)
{
	vPortFree(p);
}

//placement version
void* operator new(size_t size, void* p)
{
	(void)size;
	return p;
}
void* operator new[](size_t size, void* p)
{
	(void)size;
	return p;
}
void operator delete (void*, void*)
{
}
void operator delete[](void*, void*)
{
}