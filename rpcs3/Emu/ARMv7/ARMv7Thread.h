#pragma once
#include "Emu\CPU\CPUThread.h"

class ARMv7Thread : public CPUThread
{
public:
	ARMv7Thread();

public:
	virtual void InitRegs(); 
	virtual void InitStack();
	virtual u64 GetFreeStackSize() const;
	virtual void SetArg(const uint pos, const u64 arg);

public:
	virtual wxString RegsToString();
	virtual wxString ReadRegString(wxString reg);
	virtual bool WriteRegString(wxString reg, wxString value);

protected:
	virtual void DoReset();
	virtual void DoRun();
	virtual void DoPause();
	virtual void DoResume();
	virtual void DoStop();

	virtual void DoCode();
};