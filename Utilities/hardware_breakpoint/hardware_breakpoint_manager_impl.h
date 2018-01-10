#pragma once

#include "hardware_breakpoint_manager.h"

// Abstract interface for hardware breakpoints
class hardware_breakpoint_manager_impl
{
    friend class hardware_breakpoint_manager;

protected:
    virtual std::shared_ptr<hardware_breakpoint> set(const u32 index, const thread_handle thread, const hardware_breakpoint_type type, 
        const hardware_breakpoint_size size, const u64 address, const hardware_breakpoint_handler& handler) = 0;

    virtual bool remove(hardware_breakpoint& handle) = 0;

public:
	// Todo: move to general utility?
	inline static void set_bits(u64* value, u32 start_index, u32 bit_count, u32 new_value)
	{
		u64 mask = (1 << bit_count) - 1;
		*value = (*value & ~(mask << start_index)) | (new_value << start_index);
	}

	inline static void set_debug_control_register(u64* reg, u32 index, hardware_breakpoint_type type, hardware_breakpoint_size size, bool enable)
	{
		set_bits(reg, (index * 2), 1, static_cast<u32>(enable));
		set_bits(reg, 16 + index * 4, 2, static_cast<u32>(type));
		set_bits(reg, 18 + index * 4, 2, static_cast<u32>(size));
	}
};

