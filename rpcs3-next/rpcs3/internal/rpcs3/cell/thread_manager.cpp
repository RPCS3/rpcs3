#if 0
#include "stdafx.h"
#include "PPCThreadManager.h"
#include "PPUThread.h"
#include "SPUThread.h"
#include "RawSPUThread.h"

PPCThreadManager::PPCThreadManager() :
m_raw_spu_num(0)
{
}

PPCThreadManager::~PPCThreadManager()
{
	Close();
}

void PPCThreadManager::Close()
{
	while(m_threads.size()) RemoveThread(m_threads[0]->GetId());
}

PPCThread& PPCThreadManager::AddThread(PPCThreadType type)
{
	std::lock_guard<std::mutex> lock(m_mtx_thread);

	PPCThread* new_thread;
	char* name;
	switch(type)
	{
	case PPC_THREAD_PPU:		new_thread = new PPUThread(); name = "PPU"; break;
	case PPC_THREAD_SPU:		new_thread = new SPUThread(); name = "SPU"; break;
	case PPC_THREAD_RAW_SPU:	new_thread = new RawSPUThread(m_raw_spu_num++); name = "RawSPU"; break;
	default: assert(0);
	}
	
	new_thread->SetId(Emu.GetIdManager().GetNewID(fmt::Format("%s Thread", name), new_thread));

	m_threads.push_back(new_thread);
	wxGetApp().SendDbgCommand(DID_CREATE_THREAD, new_thread);

	return *new_thread;
}

void PPCThreadManager::RemoveThread(const u32 id)
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

		PPCThread* thr = m_threads[i];
		wxGetApp().SendDbgCommand(DID_REMOVE_THREAD, thr);
		if(thr->IsAlive())
		{
			thr->Close();
		}
		else
		{
			thr->Close();
			delete thr;
		}


		m_threads.erase(m_threads.begin() + i);
		i--;
	}

	Emu.GetIdManager().RemoveID(id, false);
	Emu.CheckStatus();
}

s32 PPCThreadManager::GetThreadNumById(PPCThreadType type, u32 id)
{
	s32 num = 0;

	for(u32 i=0; i<m_threads.size(); ++i)
	{
		if(m_threads[i]->GetId() == id) return num;
		if(m_threads[i]->GetType() == type) num++;
	}

	return -1;
}

PPCThread* PPCThreadManager::GetThread(u32 id)
{
	for(u32 i=0; i<m_threads.size(); ++i)
	{
		if(m_threads[i]->GetId() == id) return m_threads[i];
	}

	return nullptr;
}

void PPCThreadManager::Exec()
{
	for(u32 i=0; i<m_threads.size(); ++i)
	{
		m_threads[i]->Exec();
	}
}
#endif
