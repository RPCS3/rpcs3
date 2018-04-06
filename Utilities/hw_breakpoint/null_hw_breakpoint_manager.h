#pragma once

#include "hw_breakpoint_manager_impl.h"

class null_hw_breakpoint_manager final : public hw_breakpoint_manager_impl
{
protected:
	std::shared_ptr<hw_breakpoint> set(u32 index, thread_handle thread, hw_breakpoint_type type,
		hw_breakpoint_size size, u64 address, const hw_breakpoint_handler& handler)
	{
		return nullptr;
	};

	bool remove(hw_breakpoint& handle) { return false; };
};
