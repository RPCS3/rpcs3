#pragma once

#include <unordered_set>

// Regarded as a Debugger Enchantment
namespace debug
{
	// To store the pause function/call id, and let those pause there.
	// Would be with a GUI to configure those.
	class autopause
	{
		std::unordered_set<u64> m_pause_syscall;
		std::unordered_set<u32> m_pause_function;

		static autopause& get_instance();
	public:

		static void reload();
		static bool pause_syscall(u64 code);
		static bool pause_function(u32 code);
	};
}
