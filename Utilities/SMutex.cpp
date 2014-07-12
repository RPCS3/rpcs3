#include <stdafx.h>
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"

#include "Utilities/SMutex.h"

__forceinline void SM_Sleep()
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

__forceinline size_t SM_GetCurrentThreadId()
{
	return g_this_thread_id ? g_this_thread_id : g_this_thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
}

__forceinline u32 SM_GetCurrentCPUThreadId()
{
	if (CPUThread* t = GetCurrentCPUThread())
	{
		return t->GetId();
	}
	return 0;
}

__forceinline be_t<u32> SM_GetCurrentCPUThreadIdBE()
{
	return SM_GetCurrentCPUThreadId();
}
