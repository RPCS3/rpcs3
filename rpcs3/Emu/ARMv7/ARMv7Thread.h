#pragma once
#include "Emu/CPU/CPUThread.h"
#include "Emu/Memory/Memory.h"
#include "ARMv7Context.h"

class ARMv7Thread : public CPUThread
{
public:
	ARMv7Context context;
	//u32 m_arg;
	//u8 m_last_instr_size;
	//const char* m_last_instr_name;

	ARMv7Thread();

	//void update_code(const u32 address)
	//{
	//	code.code0 = vm::psv::read16(address & ~1);
	//	code.code1 = vm::psv::read16(address + 2 & ~1);
	//	m_arg = address & 0x1 ? code.code1 << 16 | code.code0 : code.data;
	//}

public:
	virtual void InitRegs(); 
	virtual void InitStack();
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
class arm7_thread : cpu_thread
{
	static const u32 stack_align = 0x10;
	vm::ptr<u64> argv;
	u32 argc;
	vm::ptr<u64> envp;

public:
	arm7_thread(u32 entry, const std::string& name = "", u32 stack_size = 0, u32 prio = 0);

	cpu_thread& args(std::initializer_list<std::string> values) override
	{
		if (!values.size())
			return *this;

		//assert(argc == 0);

		//envp.set(vm::alloc((u32)sizeof(envp), stack_align, vm::main));
		//*envp = 0;
		//argv.set(vm::alloc(u32(sizeof(argv)* values.size()), stack_align, vm::main));

		for (auto &arg : values)
		{
			//u32 arg_size = align(u32(arg.size() + 1), stack_align);
			//u32 arg_addr = vm::alloc(arg_size, stack_align, vm::main);

			//std::strcpy(vm::get_ptr<char>(arg_addr), arg.c_str());

			//argv[argc++] = arg_addr;
		}

		return *this;
	}

	cpu_thread& run() override
	{
		thread->Run();

		//static_cast<ARMv7Thread*>(thread)->GPR[0] = argc;
		//static_cast<ARMv7Thread*>(thread)->GPR[1] = argv.addr();
		//static_cast<ARMv7Thread*>(thread)->GPR[2] = envp.addr();

		return *this;
	}
};
