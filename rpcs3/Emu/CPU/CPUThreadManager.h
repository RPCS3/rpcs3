#pragma once

class CPUThread;
class RawSPUThread;
enum CPUThreadType : unsigned char;

class CPUThreadManager
{
	std::mutex m_mutex;

	std::vector<std::shared_ptr<CPUThread>> m_threads;
	std::array<std::shared_ptr<CPUThread>, 5> m_raw_spu;

public:
	CPUThreadManager();
	~CPUThreadManager();

	void Close();

	std::shared_ptr<CPUThread> AddThread(CPUThreadType type);

	void RemoveThread(u32 id);

	std::vector<std::shared_ptr<CPUThread>> GetThreads() { std::lock_guard<std::mutex> lock(m_mutex); return m_threads; }

	std::shared_ptr<CPUThread> GetThread(u32 id);
	std::shared_ptr<CPUThread> GetThread(u32 id, CPUThreadType type);
	std::shared_ptr<CPUThread> GetRawSPUThread(u32 index);

	void Exec();
	void Task();
};
