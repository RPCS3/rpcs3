#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Callback.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPCThread.h"

Callback::Callback(u32 slot, u64 addr)
	: m_addr(addr)
	, m_slot(slot)
	, a1(0)
	, a2(0)
	, a3(0)
	, a4(0)
	, a5(0)
	, m_has_data(false)
	, m_name("Callback")
{
}

u32 Callback::GetSlot() const
{
	return m_slot;
}

u64 Callback::GetAddr() const
{
	return m_addr;
}

void Callback::SetSlot(u32 slot)
{
	m_slot = slot;
}

void Callback::SetAddr(u64 addr)
{
	m_addr = addr;
}

bool Callback::HasData() const
{
	return m_has_data;
}

void Callback::Handle(u64 _a1, u64 _a2, u64 _a3, u64 _a4, u64 _a5)
{
	a1 = _a1;
	a2 = _a2;
	a3 = _a3;
	a4 = _a4;
	a5 = _a5;
	m_has_data = true;
}

u64 Callback::Branch(bool wait)
{
	m_has_data = false;

	static std::mutex cb_mutex;

	CPUThread& thr = Emu.GetCallbackThread();

again:

	while (thr.IsAlive())
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "Callback::Branch() aborted");
			return 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	std::lock_guard<std::mutex> lock(cb_mutex);

	if (thr.IsAlive())
	{
		goto again;
	}
	if (Emu.IsStopped())
	{
		LOG_WARNING(HLE, "Callback::Branch() aborted");
		return 0;
	}

	thr.Stop();
	thr.Reset();

	thr.SetEntry(m_addr);
	thr.SetPrio(1001);
	thr.SetStackSize(0x10000);
	thr.SetName(m_name);

	thr.SetArg(0, a1);
	thr.SetArg(1, a2);
	thr.SetArg(2, a3);
	thr.SetArg(3, a4);
	thr.Run();
	((PPUThread&)thr).GPR[7] = a5;

	thr.Exec();

	if (!wait)
	{
		return 0;
	}

	while (thr.IsAlive())
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "Callback::Branch(true) aborted (end)");
			return 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return thr.GetExitStatus();
}

void Callback::SetName(const std::string& name)
{
	m_name = name;
}

Callback::operator bool() const
{
	return GetAddr() != 0;
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
