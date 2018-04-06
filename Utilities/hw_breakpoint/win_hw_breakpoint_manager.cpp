
#include "stdafx.h"

#ifdef _WIN32

#include "win_hw_breakpoint_manager.h"
#include <Windows.h>

static DWORD WINAPI thread_proc(LPVOID lpParameter);

struct breakpoint_context
{
	hw_breakpoint* m_handle;
	bool m_is_setting;
	bool m_success;
};

std::shared_ptr<hw_breakpoint> win_hw_breakpoint_manager::set(u32 index,
	thread_handle thread, hw_breakpoint_type type, hw_breakpoint_size size,
	u64 address, const hw_breakpoint_handler& handler)
{
	auto handle = new hw_breakpoint(index, thread, type, size, address, handler);
	auto context = std::make_unique<breakpoint_context>();
	context->m_handle = handle;
	context->m_is_setting = true;
	context->m_success = false;

	bool should_close_thread = false;
    if (thread == GetCurrentThread())
    {
        auto pid = GetCurrentThreadId();
		handle->m_thread = OpenThread(THREAD_ALL_ACCESS, 0, pid);
		should_close_thread = true;
    }

    auto handler_thread_handle = CreateThread(0, 0, thread_proc, static_cast<LPVOID>(context.get()), 0, 0);
    WaitForSingleObject(handler_thread_handle, INFINITE);
    CloseHandle(handler_thread_handle);

    if (should_close_thread)
    {
        CloseHandle(handle->m_thread);
    }

	handle->m_thread = thread;

    if (!context->m_success)
    {
		delete handle;
        return nullptr;
    }

	return std::shared_ptr<hw_breakpoint>(handle);
};

// Removes a hardware breakpoint previously set.
bool win_hw_breakpoint_manager::remove(hw_breakpoint& handle)
{
	bool should_close_thread = false;
	if (handle.m_thread == GetCurrentThread())
	{
		auto pid = GetCurrentThreadId();
		handle.m_thread = OpenThread(THREAD_ALL_ACCESS, 0, pid);
		should_close_thread = true;
	}

	auto context = std::make_unique<breakpoint_context>();
	context->m_handle = &handle;
	context->m_is_setting = false;
	context->m_success = false;

	auto handler_thread_handle = CreateThread(0, 0, thread_proc, static_cast<LPVOID>(context.get()), 0, 0);
	WaitForSingleObject(handler_thread_handle, INFINITE);
	CloseHandle(handler_thread_handle);

	if (should_close_thread)
	{
		CloseHandle(handle.m_thread);
	}

	return context->m_success;
};

inline static void set_debug_register_value(CONTEXT* context, u32 index, u64 value)
{
	if (index == 0)
	{
		context->Dr0 = value;
	}
	else if (index == 1)
	{
		context->Dr1 = value;
	}
	else if (index == 2)
	{
		context->Dr2 = value;
	}
	else if (index == 3)
	{
		context->Dr3 = value;
	}
}

static DWORD WINAPI thread_proc(LPVOID lpParameter)
{
    auto context = static_cast<breakpoint_context*>(lpParameter);
	auto handle = context->m_handle;
	auto other_thread = handle->get_thread();

    SuspendThread(other_thread);

	// Get debug registers from other thread
    CONTEXT thread_context = {0};
    thread_context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(other_thread, &thread_context);

    if (context->m_is_setting)
    {
		// Set breakpoint address
		set_debug_register_value(&thread_context, handle->get_index(), handle->get_address());

		// Set control flags
		hw_breakpoint_manager_impl::set_debug_control_register(&thread_context.Dr7, handle->get_index(),
			handle->get_type(), handle->get_size(), true);
    }
    else
    {
		// Clear breakpoint address
		set_debug_register_value(&thread_context, handle->get_index(), 0);

		// Set control flags
		hw_breakpoint_manager_impl::set_debug_control_register(&thread_context.Dr7, handle->get_index(),
			static_cast<hw_breakpoint_type>(0), static_cast<hw_breakpoint_size>(0), false);
    }

	// Set debug registers
    thread_context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    SetThreadContext(other_thread, &thread_context);

    context->m_success = true;
    ResumeThread(other_thread);
    return 0;
}

#endif // #ifdef _WIN32
