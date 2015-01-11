#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "Callback.h"

void CallbackManager::Register(const std::function<s32(PPUThread& PPU)>& func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_cb_list.push_back([=](CPUThread& CPU) -> s32
	{
		assert(CPU.GetType() == CPU_THREAD_PPU);
		return func(static_cast<PPUThread&>(CPU));
	});
}

void CallbackManager::Async(const std::function<void(PPUThread& PPU)>& func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_async_list.push_back([=](CPUThread& CPU)
	{
		assert(CPU.GetType() == CPU_THREAD_PPU);
		func(static_cast<PPUThread&>(CPU));
	});

	m_cb_thread->Notify();
}

bool CallbackManager::Check(CPUThread& CPU, s32& result)
{
	std::function<s32(CPUThread& CPU)> func;
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
		result = func(CPU);
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

	if (Memory.PSV.RAM.GetStartAddr())
	{
		m_cb_thread = &Emu.GetCPU().AddThread(CPU_THREAD_ARMv7);
		m_cb_thread->SetName("Callback Thread");
		m_cb_thread->SetEntry(0);
		m_cb_thread->SetPrio(1001);
		m_cb_thread->SetStackSize(0x10000);
		m_cb_thread->InitStack();
		m_cb_thread->InitRegs();
		static_cast<ARMv7Thread*>(m_cb_thread)->DoRun();
	}
	else
	{
		m_cb_thread = &Emu.GetCPU().AddThread(CPU_THREAD_PPU);
		m_cb_thread->SetName("Callback Thread");
		m_cb_thread->SetEntry(0);
		m_cb_thread->SetPrio(1001);
		m_cb_thread->SetStackSize(0x10000);
		m_cb_thread->InitStack();
		m_cb_thread->InitRegs();
		static_cast<PPUThread*>(m_cb_thread)->DoRun();
	}

	thread cb_async_thread("CallbackManager::Async() thread", [this]()
	{
		SetCurrentNamedThread(m_cb_thread);

		while (!Emu.IsStopped())
		{
			std::function<void(CPUThread& CPU)> func;
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
				func(*m_cb_thread);
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
