#pragma once

#include "hardware_breakpoint_manager_impl.h"

class null_hardware_breakpoint_manager : public hardware_breakpoint_manager_impl
{
protected:
    std::shared_ptr<hardware_breakpoint> set(const u32 index, const thread_handle thread, const hardware_breakpoint_type type,
        const hardware_breakpoint_size size, const u64 address, const hardware_breakpoint_handler& handler) 
    { 
        return nullptr;
    };

    bool remove(hardware_breakpoint& handle) { return false; };
};
