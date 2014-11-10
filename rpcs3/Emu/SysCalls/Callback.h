#pragma once

class CPUThread;

class CallbackManager
{
	std::vector<std::function<s32()>> m_cb_list;
	std::vector<std::function<void()>> m_async_list;
	CPUThread* m_cb_thread;
	std::mutex m_mutex;

public:
	void Register(const std::function<s32()>& func); // register callback (called in Check() method)

	void Async(const std::function<void()>& func); // register callback for callback thread (called immediately)

	bool Check(s32& result); // call one callback registered by Register() method

	void Init();

	void Clear();
};
