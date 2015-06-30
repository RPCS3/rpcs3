#pragma once
#include "Emu/CPU/CPUThread.h"
#include "ARMv7Context.h"

class ARMv7Thread final : public CPUThread, public ARMv7Context
{
public:
	std::function<void(ARMv7Thread& CPU)> custom_task;

public:
	ARMv7Thread(const std::string& name);
	virtual ~ARMv7Thread() override;

	virtual void DumpInformation() const override;
	virtual u32 GetPC() const override { return PC; }
	virtual u32 GetOffset() const override { return 0; }
	virtual void DoRun() override;
	virtual void Task() override;

	virtual void InitRegs() override; 
	virtual void InitStack() override;
	virtual void CloseStack() override;
	u32 GetStackArg(u32 pos);
	void FastCall(u32 addr);
	void FastStop();

	virtual std::string RegsToString() const override;
	virtual std::string ReadRegString(const std::string& reg) const override;
	virtual bool WriteRegString(const std::string& reg, std::string value) override;
};

class armv7_thread : cpu_thread
{
	u32 argv;
	u32 argc;

public:
	armv7_thread(u32 entry, const std::string& name, u32 stack_size, s32 prio);

	cpu_thread& args(std::initializer_list<std::string> values) override;

	cpu_thread& run() override;
};
