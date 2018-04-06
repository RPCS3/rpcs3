
#include "stdafx.h"
#include "hw_breakpoint_manager.h"
#include "Utilities/Thread.h"
#include <thread>
#include <chrono>

#ifdef _WIN32
#include "win_hw_breakpoint_manager.h"
#elif __linux__ 
#include "linux_hw_breakpoint_manager.h"
#else
#include "null_hw_breakpoint_manager.h"
#endif

#ifdef _WIN32
std::unique_ptr<hw_breakpoint_manager_impl> hw_breakpoint_manager::s_impl = std::make_unique<win_hw_breakpoint_manager>();
#elif __linux__
std::unique_ptr<hw_breakpoint_manager_impl> hw_breakpoint_manager::s_impl = std::make_unique<linux_hw_breakpoint_manager>();
#else
std::unique_ptr<hw_breakpoint_manager_impl> hw_breakpoint_manager::s_impl = std::make_unique<null_hw_breakpoint_manager>();
#endif

thread_breakpoints_lookup hw_breakpoint_manager::s_hw_breakpoints{};
std::mutex hw_breakpoint_manager::s_mutex{};

thread_breakpoints& hw_breakpoint_manager::lookup_or_create_thread_breakpoints(thread_handle thread)
{
	auto found = s_hw_breakpoints.find(thread);
	if (found == s_hw_breakpoints.end())
	{
		// Make new entry for thread
		s_hw_breakpoints[thread] = thread_breakpoints();
		return s_hw_breakpoints[thread];
	}
	else
	{
		return found->second;
	}
}

u32 hw_breakpoint_manager::get_next_breakpoint_index(const thread_breakpoints& breakpoints)
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

extern thread_local bool g_tls_inside_exception_handler;

std::shared_ptr<hw_breakpoint> hw_breakpoint_manager::set(thread_handle thread, hw_breakpoint_type type,
	hw_breakpoint_size size, u64 address, const void* user_data, const hw_breakpoint_handler& handler)
{
	if (g_tls_inside_exception_handler)
	{
		// Can't set context (registers) inside exception handler
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(s_mutex);
	auto& breakpoints = lookup_or_create_thread_breakpoints(thread);

	// Max amount of hw breakpoints is 4
	if (breakpoints.size() >= 4)
	{
		return nullptr;
	}

	// Find next available index
	u32 index = get_next_breakpoint_index(breakpoints);

	auto handle = s_impl->set(index, thread, type, size, address, handler, user_data);
	if (handle != nullptr)
	{
		breakpoints.push_back(handle);
	}

	return handle;
};

bool hw_breakpoint_manager::remove(hw_breakpoint& handle)
{
	std::lock_guard<std::mutex> lock(s_mutex);

	auto& breakpoints = s_hw_breakpoints[handle.get_thread()];
	if (breakpoints.size() > 0)
	{
		breakpoints.erase(std::remove_if(breakpoints.begin(), breakpoints.end(), [&handle](auto x)
		{
			return x.get()->get_index() == handle.get_index();
		}), breakpoints.end());
	}

	if (g_tls_inside_exception_handler)
	{
		// Use case: Remove breakpoint while inside breakpoint handler
		// We're inside an exception handler, so we can't change the debug registers
		// so just spawn a thread that disables the breakpoint when we've left the exception
		// handler

		bool* inside_exception_handler = &g_tls_inside_exception_handler;

		thread_ctrl::spawn("hw_breakpoint_remover", [inside_exception_handler, &handle]
		{
			while (*inside_exception_handler)
				std::this_thread::sleep_for(5ms);

			s_impl->remove(handle);
		});

		return true;
	}
	else
	{
		return s_impl->remove(handle);
	}
};
