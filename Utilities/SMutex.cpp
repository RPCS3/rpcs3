#include <stdafx.h>
#include <Utilities/SMutex.h>

__forceinline void SM_Sleep()
{
	Sleep(1);
}

#ifdef _WIN32
__declspec(thread)
#else
thread_local
#endif
size_t g_this_thread_id = 0;

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
