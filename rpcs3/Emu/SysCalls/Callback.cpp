#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "ErrorCodes.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Callback.h"

void CallbackManager::Register(const std::function<s32()>& func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_cb_list.push_back(func);
}

void CallbackManager::Async(const std::function<void()>& func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_async_list.push_back(func);
	m_cb_thread->Notify();
}

bool CallbackManager::Check(s32& result)
{
	std::function<s32()> func = nullptr;

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_cb_list.size())
		{
			func = m_cb_list[0];
			m_cb_list.erase(m_cb_list.begin());
		}
	}
	
	if (func)
	{
		result = func();
		return true;
	}
	else
	{
		return false;
	}
}

void CallbackManager::Init()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_cb_thread = &Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	u32 cb_shit = Memory.MainMem.AllocAlign(8);
	vm::write32(cb_shit, Emu.m_ppu_thr_stop);
	vm::write32(cb_shit + 4, 0);

	m_cb_thread->SetName("Callback Thread");
	m_cb_thread->SetEntry(cb_shit);
	m_cb_thread->SetPrio(1001); // ???
	m_cb_thread->SetStackSize(0x10000);

	thread cb_async_thread("CallbackManager::Async() thread", [this]()
	{
		while (Emu.IsReady())
		{
			m_cb_thread->WaitForAnySignal();
		}
		
		m_cb_thread->Run();

		SetCurrentNamedThread(m_cb_thread);

		while (!Emu.IsStopped())
		{
			std::function<void()> func = nullptr;
			{
				std::lock_guard<std::mutex> lock(m_mutex);

				if (m_async_list.size())
				{
					func = m_async_list[0];
					m_async_list.erase(m_async_list.begin());
				}
			}

			if (func)
			{
				func();
				continue;
			}
			m_cb_thread->WaitForAnySignal();
		}
	});

	cb_async_thread.detach();
}

void CallbackManager::Clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_cb_list.clear();
	m_async_list.clear();
}
