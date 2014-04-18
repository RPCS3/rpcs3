#pragma once
class CPUThread;
enum CPUThreadType : unsigned char;

class CPUThreadManager
{
	std::vector<CPUThread*> m_threads;
	std::mutex m_mtx_thread;
	wxSemaphore m_sem_task;
	u32 m_raw_spu_num;

public:
	CPUThreadManager();
	~CPUThreadManager();

	void Close();

	CPUThread& AddThread(CPUThreadType type);
	void RemoveThread(const u32 id);

	std::vector<CPUThread*>& GetThreads() { return m_threads; }
	s32 GetThreadNumById(CPUThreadType type, u32 id);
	CPUThread* GetThread(u32 id);

	void Exec();
	void Task();
};
