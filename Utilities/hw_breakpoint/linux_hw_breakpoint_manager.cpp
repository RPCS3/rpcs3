
#include "stdafx.h"

#ifdef __linux__
#include "linux_hw_breakpoint_manager.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/prctl.h>
#include <cstddef>
#include <unistd.h>

// based on https://github.com/whh8b/hwbp_lib/blob/master/hwbp_lib.c
bool linux_hw_breakpoint_manager::set_debug_register_values(u32 index, pid_t thread, hw_breakpoint_type type, hw_breakpoint_size size, u32 address, bool enable)
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

		// Set parameters for when the breakpoint should be triggered.
		s64 control_register = 0;
		hw_breakpoint_manager_impl::set_debug_control_register(reinterpret_cast<u64*>(&control_register), index,
			type, size, enable);

		if (ptrace(PTRACE_POKEUSER, thread, offsetof(struct user, u_debugreg[7]), control_register))
		{
			_exit(1);
		}

		// Detach fork
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

std::shared_ptr<hw_breakpoint> linux_hw_breakpoint_manager::set(u32 index, thread_handle thread,
	hw_breakpoint_type type, hw_breakpoint_size size, u64 address, const hw_breakpoint_handler& handler, const void* user_data)
{
	if (!set_debug_register_values(index, static_cast<pid_t>(thread), type, size, address, true))
	{
		return nullptr;
	}

	return std::shared_ptr<hw_breakpoint>(new hw_breakpoint(index, thread, type, size, address, handler, user_data));
}

bool linux_hw_breakpoint_manager::remove(hw_breakpoint& handle)
{
	return set_debug_register_values(handle.get_index(), static_cast<pid_t>(handle.get_thread()), static_cast<hw_breakpoint_type>(0),
		static_cast<hw_breakpoint_size>(0), 0, false);
}

#endif // #ifdef __linux__
