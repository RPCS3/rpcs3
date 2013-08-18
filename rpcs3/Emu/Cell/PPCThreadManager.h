#pragma once
#include "PPCThread.h"

class PPCThreadManager
{
	//IdManager m_threads_id;
	//ArrayF<PPUThread> m_ppu_threads;
	//ArrayF<SPUThread> m_spu_threads;
	ArrayF<PPCThread> m_threads;
	std::mutex m_mtx_thread;
	wxSemaphore m_sem_task;
	Stack<u32> m_delete_threads;

public:
	PPCThreadManager();
	~PPCThreadManager();

	void Close();

	PPCThread& AddThread(PPCThreadType type);
	void RemoveThread(const u32 id);

	ArrayF<PPCThread>& GetThreads() { return m_threads; }
	s32 GetThreadNumById(PPCThreadType type, u32 id);
	PPCThread* GetThread(u32 id);
	//IdManager& GetIDs() {return m_threads_id;}

	void Exec();
	void Task();
};