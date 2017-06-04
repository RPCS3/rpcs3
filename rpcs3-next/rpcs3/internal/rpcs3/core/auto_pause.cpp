#include "auto_pause.h"
#include <rpcs3/core/log.h>
#include <rpcs3/core/config.h>
#include <rpcs3/core/events.h>
#include <common/file.h>
#include <memory>

namespace rpcs3
{
	inline namespace core
	{
		std::unique_ptr<auto_pause> g_autopause;

		auto_pause& auto_pause::instance(void)
		{
			if (!g_autopause)
			{
				g_autopause.reset(new auto_pause);
			}

			return *g_autopause;
		}

		//Still use binary format. Default Setting should be "disable all auto pause".
		auto_pause::auto_pause()
		{
			m_pause_function.reserve(16);
			m_pause_syscall.reserve(16);
			initialized = false;
			//Reload(false, false);
			reload();
		}

		//Notice: I would not allow to write the binary to file in this command.
		auto_pause::~auto_pause()
		{
			initialized = false;
			m_pause_function.clear();
			m_pause_syscall.clear();
			m_pause_function_enable = false;
			m_pause_syscall_enable = false;
		}

		//Load Auto Pause Configuration from file "pause.bin"
		//This would be able to create in a GUI window.
		void auto_pause::reload(void)
		{
			if (fs::is_file("pause.bin"))
			{
				m_pause_function.clear();
				m_pause_function.reserve(16);
				m_pause_syscall.clear();
				m_pause_syscall.reserve(16);

				fs::file list("pause.bin");
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
						LOG_WARNING(general, "Auto Pause: Find System Call ID 0x%x", num);
					}
					else
					{
						m_pause_function.emplace_back(num);
						LOG_WARNING(general, "Auto Pause: Find Function Call ID 0x%x", num);
					}
				}
			}

			m_pause_syscall_enable = rpcs3::config.misc.debug.auto_pause_syscall.value();
			m_pause_function_enable = rpcs3::config.misc.debug.auto_pause_func_call.value();
			initialized = true;
		}

		void auto_pause::try_pause(u32 code)
		{
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
						rpcs3::onpause();
						LOG_ERROR(general, "Auto Pause Triggered: System call 0x%x", code); // Used Error
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
						rpcs3::onpause();
						LOG_ERROR(general, "Auto Pause Triggered: Function call 0x%x", code); // Used Error
					}
				}
			}
		}
	}
}