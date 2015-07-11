#pragma once

class CPUThread;

class CallbackManager
{
	using check_cb_t = std::function<s32(CPUThread&)>;
	using async_cb_t = std::function<void(CPUThread&)>;

	std::mutex m_mutex;

	std::queue<check_cb_t> m_check_cb;
	std::queue<async_cb_t> m_async_cb;

	std::shared_ptr<CPUThread> m_cb_thread;

public:
	// register checked callback (accepts CPUThread&, returns s32)
	void Register(check_cb_t func);

	// register async callback, called in callback thread (accepts CPUThread&)
	void Async(async_cb_t func);

	// get one registered callback
	check_cb_t Check();

	void Init();

	void Clear();
};
