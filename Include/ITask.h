#pragma once
#include "FreeRTOS.h"
#include "task.h"

class iTask
{
private:
	void *m_pTaskHandle;
public:
	virtual void Run(void const *pParam) {};
};
