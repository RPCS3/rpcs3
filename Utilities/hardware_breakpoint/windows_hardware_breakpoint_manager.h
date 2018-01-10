#pragma once

#ifdef _WIN32

#include "hardware_breakpoint_manager_impl.h"

struct windows_hardware_breakpoint_context
{
	hardware_breakpoint* m_handle;
	bool m_is_setting;
	bool m_success;
};

class windows_hardware_breakpoint_manager : public hardware_breakpoint_manager_impl
{
protected:
    std::shared_ptr<hardware_breakpoint> set(const u32 index, const thread_handle thread,
        const hardware_breakpoint_type type, const hardware_breakpoint_size size, const u64 address, const hardware_breakpoint_handler& handler) final;

    bool remove(hardware_breakpoint& handle) final;
};

#endif // #ifdef _WIN32
