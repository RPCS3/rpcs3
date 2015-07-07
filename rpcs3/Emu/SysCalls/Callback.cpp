#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "Callback.h"

void CallbackManager::Register(check_cb_t func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_check_cb.emplace(std::move(func));
}

void CallbackManager::Async(async_cb_t func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!m_cb_thread)
	{
		throw EXCEPTION("Callback thread not found");
	}

	m_async_cb.emplace(std::move(func));

	m_cb_thread->cv.notify_one();
}

CallbackManager::check_cb_t CallbackManager::Check()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_check_cb.size())
	{
		check_cb_t func = std::move(m_check_cb.front());

		m_check_cb.pop();

		return func;
	}

	return nullptr;
}

void CallbackManager::Init()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto task = [this](CPUThread& cpu)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		while (true)
		{
			CHECK_EMU_STATUS;

			if (!lock) lock.lock();

			if (m_async_cb.size())
			{
				async_cb_t func = std::move(m_async_cb.front());

				m_async_cb.pop();

				if (lock) lock.unlock();

				func(cpu);

				continue;
			}

			cpu.cv.wait(lock);
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

	m_check_cb = decltype(m_check_cb){};
	m_async_cb = decltype(m_async_cb){};

	m_cb_thread.reset();
}
