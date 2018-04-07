#pragma once

#include "stdafx.h"
#include <mutex>
#include "hw_breakpoint.h"

class hw_breakpoint_manager_impl;
using thread_breakpoints = std::vector<std::shared_ptr<hw_breakpoint>>;
using thread_breakpoints_lookup = std::unordered_map<thread_handle, thread_breakpoints>;

// Static utility class for setting and removing hardware breakpoints.
class hw_breakpoint_manager final
{
private:
	hw_breakpoint_manager() {}

	static std::unique_ptr<hw_breakpoint_manager_impl> s_impl;
	static thread_breakpoints_lookup s_hw_breakpoints;
	static std::mutex s_mutex;

	static thread_breakpoints& lookup_or_create_thread_breakpoints(thread_handle thread);
	static u32 get_next_breakpoint_index(const thread_breakpoints& breakpoints);

public:

	// Gets the map containing all of the breakpoints for all threads. Unused entries are null.
	inline static const thread_breakpoints_lookup& get_breakpoints() { return s_hw_breakpoints; }

	// Gets the array of breakpoints assigned to the given thread. Unused entries are null.
	inline static const thread_breakpoints& get_breakpoints(thread_handle thread)
	{
		return s_hw_breakpoints[thread];
	}

	// Sets a hardware breakpoint for the given thread. Only up to 4 hardware breakpoints are supported.
	// Returns nullptr on failure.
	static std::shared_ptr<hw_breakpoint> set(const thread_handle thread,
		const hw_breakpoint_type type, const hw_breakpoint_size size, const u64 address,
		const void* user_data, const hw_breakpoint_handler& handler);

	// Removes a hardware breakpoint previously set.
	// The breakpoint is considered invalid after this function has been called.
	static bool remove(hw_breakpoint& breakpoint);
};
