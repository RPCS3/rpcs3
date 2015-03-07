#pragma once

class CPUThread;
class PPUThread;

typedef void(PauseResumeCB)(bool is_paused);

class CallbackManager
{
	std::mutex m_mutex;
	std::vector<std::function<s32(CPUThread&)>> m_cb_list;
	std::vector<std::function<void(CPUThread&)>> m_async_list;
	std::shared_ptr<CPUThread> m_cb_thread;

	struct PauseResumeCBS
	{
		std::function<PauseResumeCB> cb;
		u64 tag;
	};

	u64 next_tag; // not initialized, only increased
	std::vector<PauseResumeCBS> m_pause_cb_list;

public:
	void Register(const std::function<s32(PPUThread& CPU)>& func); // register callback (called in Check() method)

	void Async(const std::function<void(PPUThread& CPU)>& func); // register callback for callback thread (called immediately)

	bool Check(CPUThread& CPU, s32& result); // call one callback registered by Register() method

	void Init();

	void Clear();

	u64 AddPauseCallback(const std::function<PauseResumeCB>& func); // register callback for pausing/resuming emulation events
	void RemovePauseCallback(const u64 tag); // unregister callback (uses the result of AddPauseCallback() function)
	void RunPauseCallbacks(const bool is_paused);
};

class PauseCallbackRegisterer
{
	CallbackManager& cb_manager;
	u64 cb_tag;

private:
	PauseCallbackRegisterer() = delete;
	PauseCallbackRegisterer(const PauseCallbackRegisterer& right) = delete;
	PauseCallbackRegisterer(PauseCallbackRegisterer&& right) = delete;

	PauseCallbackRegisterer& operator =(const PauseCallbackRegisterer& right) = delete;
	PauseCallbackRegisterer& operator =(PauseCallbackRegisterer&& right) = delete;

public:
	PauseCallbackRegisterer(CallbackManager& cb_manager, const std::function<PauseResumeCB>& func)
		: cb_manager(cb_manager)
		, cb_tag(cb_manager.AddPauseCallback(func))
	{
	}

	~PauseCallbackRegisterer()
	{
		cb_manager.RemovePauseCallback(cb_tag);
	}
};
