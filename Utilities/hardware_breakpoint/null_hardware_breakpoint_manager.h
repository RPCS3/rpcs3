#pragma once

#include "hardware_breakpoint_manager_impl.h"

class null_hardware_breakpoint_manager final : public hardware_breakpoint_manager_impl
{
protected:
    std::shared_ptr<hardware_breakpoint> set(u32 index, thread_handle thread, hardware_breakpoint_type type,
        hardware_breakpoint_size size, u64 address, const hardware_breakpoint_handler& handler)
    { 
        return nullptr;
    };

    bool remove(hardware_breakpoint& handle) { return false; };
};
