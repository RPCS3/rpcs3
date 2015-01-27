#pragma once

class CPUThread;
class RawSPUThread;
enum CPUThreadType : unsigned char;

class CPUThreadManager
{
	std::vector<std::shared_ptr<CPUThread>> m_threads;
	std::mutex m_mtx_thread;

public:
	CPUThreadManager();
	~CPUThreadManager();

	void Close();

	CPUThread& AddThread(CPUThreadType type);
	void RemoveThread(const u32 id);

	//std::vector<std::shared_ptr<CPUThread>>& GetThreads() { return m_threads; }
	s32 GetThreadNumById(CPUThreadType type, u32 id);
	std::shared_ptr<CPUThread> GetThread(u32 id);
	std::shared_ptr<CPUThread> GetThread(u32 id, CPUThreadType type);
	RawSPUThread* GetRawSPUThread(u32 num);

	void Exec();
	void Task();
};
