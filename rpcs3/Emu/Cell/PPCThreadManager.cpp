#include "stdafx.h"
#include "PPCThreadManager.h"
#include "PPUThread.h"
#include "SPUThread.h"

PPCThreadManager::PPCThreadManager()
{
}

PPCThreadManager::~PPCThreadManager()
{
	Close();
}

void PPCThreadManager::Close()
{
	while(m_threads.GetCount()) RemoveThread(m_threads[0].GetId());
}

PPCThread& PPCThreadManager::AddThread(bool isPPU)
{
	PPCThread* new_thread = isPPU ? (PPCThread*)new PPUThread() : (PPCThread*)new SPUThread();

	new_thread->SetId(Emu.GetIdManager().GetNewID(
		wxString::Format("%s Thread", isPPU ? "PPU" : "SPU"), new_thread, 0)
	);

	m_threads.Add(new_thread);
	wxGetApp().SendDbgCommand(DID_CREATE_THREAD, new_thread);

	return *new_thread;
}

void PPCThreadManager::RemoveThread(const u32 id)
{
	for(u32 i=0; i<m_threads.GetCount(); ++i)
	{
		if(m_threads[i].m_wait_thread_id == id)
		{
			m_threads[i].Wait(false);
			m_threads[i].m_wait_thread_id = -1;
		}

		if(m_threads[i].GetId() != id) continue;

		wxGetApp().SendDbgCommand(DID_REMOVE_THREAD, &m_threads[i]);
		m_threads[i].Close();
		delete &m_threads[i];
		m_threads.RemoveFAt(i);
		i--;
	}

	Emu.GetIdManager().RemoveID(id, false);
	Emu.CheckStatus();
}

s32 PPCThreadManager::GetThreadNumById(bool isPPU, u32 id)
{
	s32 num = 0;

	for(u32 i=0; i<m_threads.GetCount(); ++i)
	{
		if(m_threads[i].GetId() == id) return num;
		if(m_threads[i].IsSPU() == !isPPU) num++;
	}

	return -1;
}

PPCThread* PPCThreadManager::GetThread(u32 id)
{
	for(u32 i=0; i<m_threads.GetCount(); ++i)
	{
		if(m_threads[i].GetId() == id) return &m_threads[i];
	}

	return nullptr;
}

void PPCThreadManager::Exec()
{
	for(u32 i=0; i<m_threads.GetCount(); ++i)
	{
		m_threads[i].Exec();
	}
}