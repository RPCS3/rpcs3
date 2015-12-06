#pragma once
#include <vector>
#include <common/basic_types.h>

namespace rpcs3
{
	using namespace common;

	inline namespace core
	{
		//Regarded as a Debugger Enchantment
		//To store the pause function/call id, and let those pause there.
		//Would be with a GUI to configure those.
		struct auto_pause
		{
			std::vector<u32> m_pause_syscall;
			std::vector<u32> m_pause_function;
			bool initialized;
			bool m_pause_syscall_enable;
			bool m_pause_function_enable;

			auto_pause();
			~auto_pause();
		public:
			static auto_pause& instance();

			void reload();

			void try_pause(u32 code);
		};
	}
}