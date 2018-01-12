#pragma once

#include "stdafx.h"

// Represents the type of hardware breakpoint.
enum class hardware_breakpoint_type
{
    // Breaks when the CPU tries to execute instructions
    // at the address.
	// Only valid in combination with hardware_breakpoint_size::size_1.
    executable = 0,

    // Breaks when the memory at the address is written to.
    write = 1,

	// Breaks when the memory at the address is read or
	// written to.
	read_write = 3,
};

// Represents the size of a hardware breakpoint.
// The 'size' refers to the range of bytes watched at
// the address.
enum class hardware_breakpoint_size
{
    // 1 byte.
    size_1 = 0,

    // 2 bytes.
    size_2 = 1,

    // 4 bytes.
    size_4 = 3,

    // 8 bytes.
    size_8 = 2
};

#ifdef _WIN32
using thread_handle = void*;
#else
using thread_handle = int;
#endif

class hardware_breakpoint;
using hardware_breakpoint_handler = std::function<void(hardware_breakpoint&)>;

// Represents a hardware breakpoint, read only.
class hardware_breakpoint
{
    friend class windows_hardware_breakpoint_manager;
    friend class linux_hardware_breakpoint_manager;

private:
	u32 m_index;
	thread_handle m_thread;
    hardware_breakpoint_type m_type;
    hardware_breakpoint_size m_size;
    u64 m_address;
    const hardware_breakpoint_handler m_handler;

protected:
    hardware_breakpoint(const u32 index, const thread_handle thread, const hardware_breakpoint_type type,
		const hardware_breakpoint_size size, const u64 address, const hardware_breakpoint_handler& handler)
        : m_index(index), m_thread(thread), m_type(type), m_size(size), m_address(address), m_handler(handler)
    {
    }

public:
	// Gets the index of the breakpoint (out of 4)
	inline u32 get_index() const { return m_index; }

	// Gets the thread the breakpoint applies to.
	inline thread_handle get_thread() const { return m_thread; }

    // Gets the type of breakpoint.
	inline hardware_breakpoint_type get_type() const { return m_type; }

    // Gets the size of the breakpoint.
	inline hardware_breakpoint_size get_size() const { return m_size; }

    // Gets the address of the breakpoint.
	inline u64 get_address() const { return m_address; }

    // Gets the handler of the breakpoint.
	inline const hardware_breakpoint_handler& get_handler() const { return m_handler; }
};

class hardware_breakpoint_manager_impl;
using thread_breakpoints = std::vector<std::shared_ptr<hardware_breakpoint>>;
using thread_breakpoints_lookup = std::unordered_map<thread_handle, thread_breakpoints>;

// Static utility class for setting and removing hardware breakpoints.
class hardware_breakpoint_manager final
{
private:
	hardware_breakpoint_manager() {}

    static std::unique_ptr<hardware_breakpoint_manager_impl> s_impl;
	static thread_breakpoints_lookup s_hardware_breakpoints;

	static thread_breakpoints& lookup_or_create_thread_breakpoints(thread_handle thread);
	static u32 get_next_breakpoint_index(const thread_breakpoints& breakpoints);

public:

	// Gets the map containing all of the breakpoints for all threads. Unused entries are null.
	inline static const thread_breakpoints_lookup& get_breakpoints() { return s_hardware_breakpoints; }

	// Gets the array of breakpoints assigned to the given thread. Unused entries are null.
	inline static const thread_breakpoints& get_breakpoints(thread_handle thread) 
	{
		return s_hardware_breakpoints[thread];
	}

    // Sets a hardware breakpoint for the given thread. Only up to 4 hardware breakpoints are supported.
    // Returns nullptr on failure.
    static std::shared_ptr<hardware_breakpoint> set(const thread_handle thread,
		const hardware_breakpoint_type type, const hardware_breakpoint_size size, const u64 address,
		const hardware_breakpoint_handler& handler);

    // Removes a hardware breakpoint previously set.
	// The breakpoint is considered invalid after this function has been called.
    static bool remove(hardware_breakpoint& breakpoint);
};
