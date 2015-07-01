#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Callback.h"

void CallbackManager::Register(std::function<s32(PPUThread& PPU)> func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_cb_list.push_back([=](CPUThread& CPU) -> s32
	{
		if (CPU.GetType() != CPU_THREAD_PPU) throw EXCEPTION("PPU thread expected");
		return func(static_cast<PPUThread&>(CPU));
	});
}

void CallbackManager::Async(std::function<void(PPUThread& PPU)> func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_async_list.push_back([=](CPUThread& CPU)
	{
		if (CPU.GetType() != CPU_THREAD_PPU) throw EXCEPTION("PPU thread expected");
		func(static_cast<PPUThread&>(CPU));
	});

	m_cv.notify_one();
}

//void CallbackManager::Async(std::function<void(ARMv7Context& context)> func)
//{
//	std::lock_guard<std::mutex> lock(m_mutex);
//
//	m_async_list.push_back([=](CPUThread& CPU)
//	{
//		if (CPU.GetType() != CPU_THREAD_ARMv7) throw EXCEPTION("ARMv7 thread expected");
//		func(static_cast<ARMv7Thread&>(CPU));
//	});
//
//	m_cv.notify_one();
//}

bool CallbackManager::Check(CPUThread& CPU, s32& result)
{
	std::function<s32(CPUThread& CPU)> func;

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_cb_list.size())
		{
			func = std::move(m_cb_list.front());
			m_cb_list.erase(m_cb_list.begin());
		}
	}
	
	return func ? result = func(CPU), true : false;
}

void CallbackManager::Init()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto task = [this](CPUThread& CPU)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		while (!CPU.CheckStatus())
		{
			std::function<void(CPUThread& CPU)> func;

			if (m_async_list.size())
			{
				func = std::move(m_async_list.front());
				m_async_list.erase(m_async_list.begin());
			}

			if (func)
			{
				if (lock) lock.unlock();

				func(*m_cb_thread);
				continue;
			}

			if (!lock) lock.lock();

			m_cv.wait_for(lock, std::chrono::milliseconds(1));
		}
	};

	if (Memory.PSV.RAM.GetStartAddr())
	{
		auto thread = Emu.GetIdManager().make_ptr<ARMv7Thread>("Callback Thread");

		thread->prio = 1001;
		thread->stack_size = 0x10000;
		thread->custom_task = task;
		thread->Run();

		m_cb_thread = thread;
	}
	else
	{
		auto thread = Emu.GetIdManager().make_ptr<PPUThread>("Callback Thread");

		thread->prio = 1001;
		thread->stack_size = 0x10000;
		thread->custom_task = task;
		thread->Run();

		m_cb_thread = thread;
	}
}

void CallbackManager::Clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_cb_list.clear();
	m_async_list.clear();
	m_pause_cb_list.clear();

	m_cb_thread.reset();
}

u64 CallbackManager::AddPauseCallback(std::function<PauseResumeCB> func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_pause_cb_list.push_back({ func, next_tag });
	return next_tag++;
}

void CallbackManager::RemovePauseCallback(const u64 tag)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	
	for (auto& data : m_pause_cb_list)
	{
		if (data.tag == tag)
		{
			m_pause_cb_list.erase(m_pause_cb_list.begin() + (&data - m_pause_cb_list.data()));
			return;
		}
	}

	assert(!"CallbackManager()::RemovePauseCallback(): tag not found");
}

void CallbackManager::RunPauseCallbacks(const bool is_paused)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& data : m_pause_cb_list)
	{
		if (data.cb)
		{
			data.cb(is_paused);
		}
	}
}
