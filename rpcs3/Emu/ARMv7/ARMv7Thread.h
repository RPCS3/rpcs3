#pragma once
#include "Emu/CPU/CPUThread.h"
#include "ARMv7Context.h"

class ARMv7Thread : public CPUThread
{
public:
	ARMv7Context context;

	ARMv7Thread();
	~ARMv7Thread();

public:
	virtual void InitRegs(); 
	virtual void InitStack();
	virtual void CloseStack();
	u32 GetStackArg(u32 pos);
	void FastCall(u32 addr);
	void FastStop();
	virtual void DoRun();

public:
	virtual std::string RegsToString();
	virtual std::string ReadRegString(const std::string& reg);
	virtual bool WriteRegString(const std::string& reg, std::string value);

protected:
	virtual void DoReset();
	virtual void DoPause();
	virtual void DoResume();
	virtual void DoStop();

	virtual void DoCode();
};

class armv7_thread : cpu_thread
{
	u32 argv;
	u32 argc;

public:
	armv7_thread(u32 entry, const std::string& name = "", u32 stack_size = 0, u32 prio = 0);

	cpu_thread& args(std::initializer_list<std::string> values) override;

	cpu_thread& run() override;
};
