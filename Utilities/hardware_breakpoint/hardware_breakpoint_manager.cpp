
#include "stdafx.h"
#include "hardware_breakpoint_manager.h"
#include "Utilities/Thread.h"
#include <thread>
#include <chrono>

#ifdef _WIN32
#include "windows_hardware_breakpoint_manager.h"
#elif __linux__ 
#include "linux_hardware_breakpoint_manager.h"
#else
#include "null_hardware_breakpoint_manager.h"
#endif

#ifdef _WIN32
std::unique_ptr<hardware_breakpoint_manager_impl> hardware_breakpoint_manager::s_impl = std::make_unique<windows_hardware_breakpoint_manager>();
#elif __linux__
std::unique_ptr<hardware_breakpoint_manager_impl> hardware_breakpoint_manager::s_impl = std::make_unique<linux_hardware_breakpoint_manager>();
#else
std::unique_ptr<hardware_breakpoint_manager_impl> hardware_breakpoint_manager::s_impl = std::make_unique<null_hardware_breakpoint_manager>();
#endif

thread_breakpoints_lookup hardware_breakpoint_manager::s_hardware_breakpoints{};

thread_breakpoints& hardware_breakpoint_manager::lookup_or_create_thread_breakpoints(thread_handle thread)
{
	auto found = s_hardware_breakpoints.find(thread);
	if (found == s_hardware_breakpoints.end())
	{
		// Make new entry for thread
		s_hardware_breakpoints[thread] = thread_breakpoints();
		return s_hardware_breakpoints[thread];
	}
	else
	{
		return found->second;
	}
}

u32 hardware_breakpoint_manager::get_next_breakpoint_index(thread_breakpoints& breakpoints)
{
	u32 index = 0;
	while (true)
	{
		bool found_next_index = true;
		for (auto breakpoint : breakpoints)
		{
			if (breakpoint->get_index() == index)
			{
				found_next_index = false;
				break;
			}
		}

		if (found_next_index)
		{
			break;
		}
		else
		{
			++index;
		}
	}

	return index;
}

extern thread_local bool g_inside_exception_handler;

std::shared_ptr<hardware_breakpoint> hardware_breakpoint_manager::set(const thread_handle thread, const hardware_breakpoint_type type, 
    const hardware_breakpoint_size size, u64 address, const hardware_breakpoint_handler& handler)
{
	if (g_inside_exception_handler)
	{
		// Can't set context (registers) inside exception handler
		return nullptr;
	}

	auto& breakpoints = lookup_or_create_thread_breakpoints(thread);

	// Max amount of hw breakpoints is 4
	if (breakpoints.size() >= 4)
	{
		return nullptr;
	}

	// Find next available index
	u32 index = get_next_breakpoint_index(breakpoints);

	auto handle = s_impl->set(index, thread, type, size, address, handler);
	if (handle != nullptr)
	{
		breakpoints.push_back(handle);
	}

	return handle;
};

bool hardware_breakpoint_manager::remove(hardware_breakpoint& handle)
{
	auto& breakpoints = get_breakpoints(handle.get_thread());
	if (breakpoints.size() > 0)
	{
		breakpoints.erase(std::remove_if(breakpoints.begin(), breakpoints.end(), [&](std::shared_ptr<hardware_breakpoint> x) { return x.get()->get_index() == handle.get_index(); }), breakpoints.end());
	}

	if (g_inside_exception_handler)
	{
		// We're inside an exception handler, so we can't change the debug registers
		// so just spawn a thread that disables the breakpoint when we've left the exception
		// handler
		bool* inside_exception_handler = &g_inside_exception_handler;

		thread_ctrl::spawn("hardware_breakpoint_remover", [&]()
		{
			while (*inside_exception_handler)
				std::this_thread::sleep_for(std::chrono::nanoseconds(100));

			s_impl->remove(handle);
		});

		return true;
	}
	else
	{
		return s_impl->remove(handle);
	}
};
