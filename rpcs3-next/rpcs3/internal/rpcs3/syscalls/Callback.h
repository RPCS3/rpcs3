#pragma once

class PPUThread;

class CallbackManager
{
	using check_cb_t = std::function<s32(PPUThread&)>;
	using async_cb_t = std::function<void(PPUThread&)>;

	std::mutex m_mutex;

	std::queue<check_cb_t> m_check_cb;
	std::queue<async_cb_t> m_async_cb;

	std::shared_ptr<PPUThread> m_cb_thread;

public:
	// Register checked callback
	void Register(check_cb_t func);

	// Register async callback, called in callback thread
	void Async(async_cb_t func);

	// Get one registered callback
	check_cb_t Check();

	void Init();

	void Clear();
};
