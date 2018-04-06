#pragma once
#include "stdafx.h"
#include "Emu/CPU/CPUThread.h"

// Represents the type of hardware breakpoint.
enum class hw_breakpoint_type
{
	// Breaks when the CPU tries to execute instructions
	// at the address.
	// Only valid in combination with hw_breakpoint_size::size_1.
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
enum class hw_breakpoint_size
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

class hw_breakpoint;
using hw_breakpoint_handler = std::function<void(const cpu_thread* thread, hw_breakpoint& breakpoint, void* user_data)>;

// Represents a hardware breakpoint, read only.
class hw_breakpoint
{
	friend class win_hw_breakpoint_manager;
	friend class linux_hw_breakpoint_manager;

private:
	u32 m_index;
	thread_handle m_thread;
	hw_breakpoint_type m_type;
	hw_breakpoint_size m_size;
	u64 m_address;
	const hw_breakpoint_handler m_handler;

protected:
	hw_breakpoint(const u32 index, const thread_handle thread, const hw_breakpoint_type type,
		const hw_breakpoint_size size, const u64 address, const hw_breakpoint_handler& handler)
		: m_index(index), m_thread(thread), m_type(type), m_size(size), m_address(address), m_handler(handler)
	{
	}

public:
	// Gets the index of the breakpoint (out of 4)
	inline u32 get_index() const { return m_index; }

	// Gets the thread the breakpoint applies to.
	inline thread_handle get_thread() const { return m_thread; }

	// Gets the type of breakpoint.
	inline hw_breakpoint_type get_type() const { return m_type; }

	// Gets the size of the breakpoint.
	inline hw_breakpoint_size get_size() const { return m_size; }

	// Gets the address of the breakpoint.
	inline u64 get_address() const { return m_address; }

	// Gets the handler of the breakpoint.
	inline const hw_breakpoint_handler& get_handler() const { return m_handler; }
};