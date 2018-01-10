
#include "stdafx.h"

#ifdef __linux__
#include "linux_hardware_breakpoint_manager.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/prctl.h>
#include <cstddef>

// based on https://github.com/whh8b/hwbp_lib/blob/master/hwbp_lib.c
// untested
bool linux_hardware_breakpoint_manager::set_debug_register_values(u32 index, pid_t thread, hardware_breakpoint_type type, hardware_breakpoint_size size, u32 address, bool enable)
{
	s32 child_status = 0;

	pid_t child;
	if (!(child = fork()))
	{
		// Child process
		s32 parent_status;
		if (ptrace(PTRACE_ATTACH, thread, nullptr, nullptr))
		{
			_exit(1);
		}

		while (!WIFSTOPPED(parent_status))
			waitpid(thread, &parent_status, 0);

		// Set the breakpoint address.
		if (ptrace(PTRACE_POKEUSER, thread, offsetof(struct user, u_debugreg[index]), address))
		{
			_exit(1);
		}

		// Fetch control register data
		s64 control_register = ptrace(PTRACE_PEEKUSER, thread, offsetof(struct user, u_debugreg[7]));
		if (control_register == -1)
			_exit(1);

		// Set control flags
		hardware_breakpoint_manager_impl::set_debug_control_register(reinterpret_cast<u64*>(&control_register), index,
			type, size, enable);

		// Set control register value
		if (ptrace(PTRACE_POKEUSER, thread, offsetof(struct user, u_debugreg[7]), control_register))
		{
			_exit(1);
		}

		if (ptrace(PTRACE_DETACH, thread, nullptr, nullptr))
		{
			_exit(1);
		}

		_exit(0);
	}

	waitpid(child, &child_status, 0);

	if (WIFEXITED(child_status) && !WEXITSTATUS(child_status))
	{
		return true;
	}

	return false;
}

std::shared_ptr<hardware_breakpoint> linux_hardware_breakpoint_manager::set(const u32 index, const thread_handle thread,
	const hardware_breakpoint_type type, const hardware_breakpoint_size size, const u64 address, const hardware_breakpoint_handler& handler)
{
	if (!set_debug_register_values(index, static_cast<pid_t>(thread), type, size, address, true))
	{
		return nullptr;
	}

	return std::shared_ptr<hardware_breakpoint>(new hardware_breakpoint(index, thread, type, size, address, handler));
}

bool linux_hardware_breakpoint_manager::remove(hardware_breakpoint& handle)
{
	return set_debug_register_values(handle.get_index(), static_cast<pid_t>(handle.get_thread()), static_cast<hardware_breakpoint_type>(0),
		static_cast<hardware_breakpoint_size>(0), 0, false);
}

#endif // #ifdef __linux__
