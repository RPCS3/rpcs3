#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"

#include "Utilities/SMutex.h"

bool SM_IsAborted()
{
	return Emu.IsStopped();
}

void SM_Sleep()
{
	if (NamedThreadBase* t = GetCurrentNamedThread())
	{
		t->WaitForAnySignal();
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

thread_local size_t g_this_thread_id = 0;

size_t SM_GetCurrentThreadId()
{
	return g_this_thread_id ? g_this_thread_id : g_this_thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
}

u32 SM_GetCurrentCPUThreadId()
{
	if (CPUThread* t = GetCurrentCPUThread())
	{
		return t->GetId();
	}
	return 0;
}

be_t<u32> SM_GetCurrentCPUThreadIdBE()
{
	return be_t<u32>::MakeFromLE(SM_GetCurrentCPUThreadId());
}
