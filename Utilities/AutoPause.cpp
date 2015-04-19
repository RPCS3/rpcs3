#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "AutoPause.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/System.h"

using namespace Debug;

//Even different from those tutorials on webpages, i use the method similiar from Log.h
//TODO:: Question: Does such a Singleton struct get fully deallocated after its pointer?
AutoPause* gAutoPause = nullptr;

AutoPause& AutoPause::getInstance(void)
{
	if (!gAutoPause)
	{
		gAutoPause = new AutoPause();
	}
	return *gAutoPause;
}

//Still use binary format. Default Setting should be "disable all auto pause".
AutoPause::AutoPause(void)
{
	m_pause_function.reserve(16);
	m_pause_syscall.reserve(16);
	initialized = false;
	//Reload(false, false);
	Reload();
}

//Notice: I would not allow to write the binary to file in this command.
AutoPause::~AutoPause(void)
{
	initialized = false;
	m_pause_function.clear();
	m_pause_syscall.clear();
	m_pause_function_enable = false;
	m_pause_syscall_enable = false;
}

//Load Auto Pause Configuration from file "pause.bin"
//This would be able to create in a GUI window.
void AutoPause::Reload(void)
{
	if (rExists("pause.bin"))
	{
		m_pause_function.clear();
		m_pause_function.reserve(16);
		m_pause_syscall.clear();
		m_pause_syscall.reserve(16);

		rfile_t list("pause.bin");
		//System calls ID and Function calls ID are all u32 iirc.
		u32 num;
		size_t fmax = list.size();
		size_t fcur = 0;
		list.seek(0);
		while (fcur <= fmax - sizeof(u32))
		{
			list.read(&num, sizeof(u32));
			fcur += sizeof(u32);
			if (num == 0xFFFFFFFF) break;
			
			if (num < 1024)
			{
				//Less than 1024 - be regarded as a system call.
				//emplace_back may not cause reductant move/copy operation.
				m_pause_syscall.emplace_back(num);
				LOG_WARNING(HLE, "Auto Pause: Find System Call ID 0x%x", num);
			}
			else
			{
				m_pause_function.emplace_back(num);
				LOG_WARNING(HLE, "Auto Pause: Find Function Call ID 0x%x", num);
			}
		}
	}
	else
	{
		LOG_WARNING(HLE, "No pause.bin found, Auto Pause will not work.");
	}
	m_pause_syscall_enable = Ini.DBGAutoPauseSystemCall.GetValue();
	m_pause_function_enable = Ini.DBGAutoPauseFunctionCall.GetValue();
	initialized = true;
}

void AutoPause::TryPause(u32 code) {
	if (code < 1024)
	{
		//Would first check Enable setting. Then the list length.
		if ((!m_pause_syscall_enable)
			|| (m_pause_syscall.size() <= 0))
		{
			return;
		}

		for (u32 i = 0; i < m_pause_syscall.size(); ++i)
		{
			if (code == m_pause_syscall[i])
			{
				Emu.Pause();
				LOG_ERROR(HLE, "Auto Pause Triggered: System call 0x%x", code); // Used Error
			}
		}
	}
	else
	{
		//Well similiar.. Seperate the list caused by possible setting difference.
		if ((!m_pause_function_enable)
			|| (m_pause_function.size() <= 0))
		{
			return;
		}

		for (u32 i = 0; i < m_pause_function.size(); ++i)
		{
			if (code == m_pause_function[i])
			{
				Emu.Pause();
				LOG_ERROR(HLE, "Auto Pause Triggered: Function call 0x%x", code); // Used Error
			}
		}
	}
}