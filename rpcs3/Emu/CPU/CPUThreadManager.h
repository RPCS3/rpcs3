#pragma once

class CPUThread;
class RawSPUThread;

class CPUThreadManager final
{
	std::mutex m_mutex;

	std::array<std::weak_ptr<RawSPUThread>, 5> m_raw_spu;

public:
	CPUThreadManager();
	~CPUThreadManager();

	void Close();

	static std::vector<std::shared_ptr<CPUThread>> GetAllThreads();

	static void Exec();

	std::shared_ptr<RawSPUThread> NewRawSPUThread();

	std::shared_ptr<RawSPUThread> GetRawSPUThread(u32 index);
};
