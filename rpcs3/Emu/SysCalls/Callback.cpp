#include "stdafx.h"
#include "Callback.h"

#include "Emu/Cell/PPCThread.h"

Callback::Callback(u32 slot, u64 addr)
	: m_addr(addr)
	, m_slot(slot)
	, a1(0)
	, a2(0)
	, a3(0)
	, m_has_data(false)
{
}

void Callback::Handle(u64 _a1, u64 _a2, u64 _a3)
{
	a1 = _a1;
	a2 = _a2;
	a3 = _a3;
	m_has_data = true;
}

void Callback::Branch()
{
	m_has_data = false;

	PPCThread& new_thread = Emu.GetCPU().AddThread(PPC_THREAD_PPU);

	new_thread.SetPc(m_addr);
	new_thread.SetPrio(1001);
	new_thread.stack_size = 0x10000;
	new_thread.SetName("Callback");

	new_thread.SetArg(0, a1);
	new_thread.SetArg(1, a2);
	new_thread.SetArg(2, a3);
	((PPUThread&)new_thread).LR = Emu.GetPPUThreadExit();
	new_thread.Run();

	new_thread.Exec();

	GetCurrentPPCThread()->Wait(new_thread);
}

Callback2::Callback2(u32 slot, u64 addr, u64 userdata) : Callback(slot, addr)
{
	a2 = userdata;
}

void Callback2::Handle(u64 status)
{
	Callback::Handle(status, a2, 0);
}

Callback3::Callback3(u32 slot, u64 addr, u64 userdata) : Callback(slot, addr)
{
	a3 = userdata;
}

void Callback3::Handle(u64 status, u64 param)
{
	Callback::Handle(status, param, a3);
}
