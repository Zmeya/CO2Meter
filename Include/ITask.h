#pragma once
#include "FreeRTOS.h"
#include "task.h"

#ifdef _MSC_VER
typedef unsigned __int8		uint8_t;
typedef unsigned __int16	uint16_t;
typedef unsigned __int32	uint32_t;
#endif

class iTask
{
private:
	void *m_pTaskHandle;
public:
	virtual void Run(void const *pParam) {};
};
