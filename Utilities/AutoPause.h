#pragma once
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/System.h"

//Regarded as a Debugger Enchantment
namespace Debug {
	//To store the pause function/call id, and let those pause there.
	//Would be with a GUI to configure those.
	struct AutoPause
	{
		std::vector<u32> m_pause_syscall;
		std::vector<u32> m_pause_function;
		bool initialized;
		bool m_pause_syscall_enable;
		bool m_pause_function_enable;

		AutoPause();
		~AutoPause();
	public:
		static AutoPause& getInstance(void);

		void Reload(void);

		void TryPause(u32 code);
	};
}