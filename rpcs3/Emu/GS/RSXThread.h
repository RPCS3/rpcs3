#pragma once
#include "GCM.h"

class RSXThread : public ThreadBase
{
	Array<u32> call_stack;
	void (&DoCmd)(const u32 fcmd, const u32 cmd, mem32_t& args, const u32 count);
	bool (&flip)();
	bool (&TestExit)();
	CellGcmControl& m_ctrl;
	u32 m_ioAddress;

protected:
	RSXThread(
		CellGcmControl* ctrl,
		u32 ioAddress, 
		void (&cmd)(const u32 fcmd, const u32 cmd, mem32_t& args, const u32 count),
		bool (&_flip)(),
		bool (&_TestExit)()
		);

private:
	virtual void Task();
	virtual void OnExit();
};