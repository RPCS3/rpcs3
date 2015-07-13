#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/DbgCommand.h"

#include "Emu/IdManager.h"
#include "CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"

CPUThreadManager::CPUThreadManager()
{
}

CPUThreadManager::~CPUThreadManager()
{
	Close();
}

void CPUThreadManager::Close()
{
	while(m_threads.size()) RemoveThread(m_threads[0]->GetId());
}

std::shared_ptr<CPUThread> CPUThreadManager::AddThread(CPUThreadType type)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::shared_ptr<CPUThread> new_thread;

	switch(type)
	{
	case CPU_THREAD_PPU:
	{
		new_thread = std::make_shared<PPUThread>();
		break;
	}
	case CPU_THREAD_SPU:
	{
		new_thread = std::make_shared<SPUThread>();
		break;
	}
	case CPU_THREAD_RAW_SPU:
	{
		for (u32 i = 0; i < m_raw_spu.size(); i++)
		{
			if (!m_raw_spu[i])
			{
				new_thread = std::make_shared<RawSPUThread>();
				new_thread->index = i;
				
				m_raw_spu[i] = new_thread;
				break;
			}
		}
		break;
	}
	case CPU_THREAD_ARMv7:
	{
		new_thread.reset(new ARMv7Thread());
		break;
	}
	default: assert(0);
	}

	if (new_thread)
	{
		new_thread->SetId(Emu.GetIdManager().add(new_thread));

		m_threads.push_back(new_thread);
		SendDbgCommand(DID_CREATE_THREAD, new_thread.get());
	}

	return new_thread;
}

void CPUThreadManager::RemoveThread(u32 id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::shared_ptr<CPUThread> thr;
	u32 thread_index = 0;

	for (u32 i = 0; i < m_threads.size(); ++i)
	{
		if (m_threads[i]->GetId() != id) continue;

		thr = m_threads[i];
		thread_index = i;
	}

	if (thr)
	{
		SendDbgCommand(DID_REMOVE_THREAD, thr.get());
		thr->Close();

		m_threads.erase(m_threads.begin() + thread_index);

		if (thr->GetType() == CPU_THREAD_RAW_SPU)
		{
			assert(thr->index < m_raw_spu.size());
			m_raw_spu[thr->index] = nullptr;
		}
	}

	// Removing the ID should trigger the actual deletion of the thread
	Emu.GetIdManager().remove<CPUThread>(id);
	Emu.CheckStatus();
}

std::shared_ptr<CPUThread> CPUThreadManager::GetThread(u32 id)
{
	return Emu.GetIdManager().get<CPUThread>(id);
}

std::shared_ptr<CPUThread> CPUThreadManager::GetThread(u32 id, CPUThreadType type)
{
	const auto res = GetThread(id);

	return res && res->GetType() == type ? res : nullptr;
}

std::shared_ptr<CPUThread> CPUThreadManager::GetRawSPUThread(u32 index)
{
	if (index >= m_raw_spu.size())
	{
		return nullptr;
	}

	return m_raw_spu[index];
}

void CPUThreadManager::Exec()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for(u32 i = 0; i < m_threads.size(); ++i)
	{
		m_threads[i]->Exec();
	}
}
