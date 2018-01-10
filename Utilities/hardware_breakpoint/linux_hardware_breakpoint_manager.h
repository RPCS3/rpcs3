#pragma once

#ifdef __linux__

#include "hardware_breakpoint_manager_impl.h"

class linux_hardware_breakpoint_manager : public hardware_breakpoint_manager_impl
{
protected:
	std::shared_ptr<hardware_breakpoint> set(const u32 index, const thread_handle thread, const hardware_breakpoint_type type,
		const hardware_breakpoint_size size, const u64 address, const hardware_breakpoint_handler& handler) final;

	bool remove(hardware_breakpoint& handle) final;

	bool set_debug_register_values(u32 index, pid_t thread, hardware_breakpoint_type type, hardware_breakpoint_size size, u32 address, bool enable);
};

#endif // #ifdef __linux__
