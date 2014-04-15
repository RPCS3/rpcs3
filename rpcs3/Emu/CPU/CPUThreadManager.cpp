#include "stdafx.h"
#include "CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"

CPUThreadManager::CPUThreadManager()
	: m_raw_spu_num(0)
{
}

CPUThreadManager::~CPUThreadManager()
{
	Close();
}

void CPUThreadManager::Close()
{
	m_raw_spu_num = 0;
	while(m_threads.size()) RemoveThread(m_threads[0]->GetId());
}

CPUThread& CPUThreadManager::AddThread(CPUThreadType type)
{
	std::lock_guard<std::mutex> lock(m_mtx_thread);

	CPUThread* new_thread;

	switch(type)
	{
	case CPU_THREAD_PPU:     new_thread = new PPUThread(); break;
	case CPU_THREAD_SPU:     new_thread = new SPUThread(); break;
	case CPU_THREAD_RAW_SPU: new_thread = new RawSPUThread(m_raw_spu_num++); break;
	case CPU_THREAD_ARMv7:   new_thread = new ARMv7Thread(); break;
	default: assert(0);
	}
	
	new_thread->SetId(Emu.GetIdManager().GetNewID(fmt::Format("%s Thread", new_thread->GetTypeString().c_str()), new_thread));

	m_threads.push_back(new_thread);
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_CREATE_THREAD, new_thread);
#endif

	return *new_thread;
}

//TODO: find out where the thread is actually deleted because it's sure as shit not here
void CPUThreadManager::RemoveThread(const u32 id)
{
	std::lock_guard<std::mutex> lock(m_mtx_thread);

	for(u32 i=0; i<m_threads.size(); ++i)
	{
		if(m_threads[i]->m_wait_thread_id == id)
		{
			m_threads[i]->Wait(false);
			m_threads[i]->m_wait_thread_id = -1;
		}

		if(m_threads[i]->GetId() != id) continue;

		CPUThread* thr = m_threads[i];
#ifndef QT_UI
		wxGetApp().SendDbgCommand(DID_REMOVE_THREAD, thr);
#endif
		thr->Close();

		m_threads.erase(m_threads.begin()+ i);
		break;
	}

	Emu.GetIdManager().RemoveID(id);
	Emu.CheckStatus();
}

s32 CPUThreadManager::GetThreadNumById(CPUThreadType type, u32 id)
{
	std::lock_guard<std::mutex> lock(m_mtx_thread);

	s32 num = 0;

	for(u32 i=0; i<m_threads.size(); ++i)
	{
		if(m_threads[i]->GetId() == id) return num;
		if(m_threads[i]->GetType() == type) num++;
	}

	return -1;
}

CPUThread* CPUThreadManager::GetThread(u32 id)
{
	CPUThread* res;

	if (!id) return nullptr;

	if (!Emu.GetIdManager().GetIDData(id, res)) return nullptr;

	return res;
}

void CPUThreadManager::Exec()
{
	std::lock_guard<std::mutex> lock(m_mtx_thread);

	for(u32 i=0; i<m_threads.size(); ++i)
	{
		m_threads[i]->Exec();
	}
}
