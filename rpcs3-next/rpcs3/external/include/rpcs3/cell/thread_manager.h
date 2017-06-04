#pragma once
#include "PPCThread.h"

enum PPCThreadType
{
	PPC_THREAD_PPU,
	PPC_THREAD_SPU,
	PPC_THREAD_RAW_SPU
};

class PPCThreadManager
{
	//IdManager m_threads_id;
	//ArrayF<PPUThread> m_ppu_threads;
	//ArrayF<SPUThread> m_spu_threads;
	std::vector<PPCThread *> m_threads;
	std::mutex m_mtx_thread;
	wxSemaphore m_sem_task;
	u32 m_raw_spu_num;

public:
	PPCThreadManager();
	~PPCThreadManager();

	void Close();

	PPCThread& AddThread(PPCThreadType type);
	void RemoveThread(const u32 id);

	std::vector<PPCThread *>& GetThreads() { return m_threads; }
	s32 GetThreadNumById(PPCThreadType type, u32 id);
	PPCThread* GetThread(u32 id);
	//IdManager& GetIDs() {return m_threads_id;}

	void Exec();
	void Task();
};
