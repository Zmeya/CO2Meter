#pragma once
#include "ITask.h"
class CCO2Task : public iTask
{
private:
	static CCO2Task *m_Instance;
public:
	CCO2Task();
	~CCO2Task();
	void Run(void const *pParam);

	static CCO2Task * GetInstance(void);
};

