#pragma once

#include "cmsis_os.h"
#include "iTask.h"

class CMainTask : public iTask
{
protected:
	static CMainTask *m_Instance;
private:
public:
	CMainTask();
	~CMainTask();
	void Run(void const *pParam);

	static CMainTask * GetInstance(void);
};