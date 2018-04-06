#pragma once

#ifdef __linux__

#include "hw_breakpoint_manager_impl.h"

class linux_hw_breakpoint_manager final : public hw_breakpoint_manager_impl
{
protected:
	std::shared_ptr<hw_breakpoint> set(u32 index, thread_handle thread, hw_breakpoint_type type,
		hw_breakpoint_size size, u64 address, const hw_breakpoint_handler& handler) final;

	bool remove(hw_breakpoint& handle) final;

	bool set_debug_register_values(u32 index, pid_t thread, hw_breakpoint_type type, hw_breakpoint_size size, u32 address, bool enable);
};

#endif // #ifdef __linux__
