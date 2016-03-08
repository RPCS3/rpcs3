#pragma once

#include "Loader/Loader.h"
#include "DbgCommand.h"

enum class frame_type;

struct EmuCallbacks
{
	std::function<void(std::function<void()>)> call_after;
	std::function<void()> process_events;
	std::function<void(DbgCommand, class CPUThread*)> send_dbg_command;
	std::function<std::unique_ptr<class KeyboardHandlerBase>()> get_kb_handler;
	std::function<std::unique_ptr<class MouseHandlerBase>()> get_mouse_handler;
	std::function<std::unique_ptr<class PadHandlerBase>()> get_pad_handler;
	std::function<std::unique_ptr<class GSFrameBase>(frame_type)> get_gs_frame;
	std::function<std::shared_ptr<class GSRender>()> get_gs_render;
	std::function<std::shared_ptr<class AudioThread>()> get_audio;
	std::function<std::shared_ptr<class MsgDialogBase>()> get_msg_dialog;
	std::function<std::unique_ptr<class SaveDialogBase>()> get_save_dialog;
};

enum Status : u32
{
	Running,
	Paused,
	Stopped,
	Ready,
};

// Emulation Stopped exception event
class EmulationStopped {};

class CPUThreadManager;
class PadManager;
class KeyboardManager;
class MouseManager;
class GSManager;
class AudioManager;
class CallbackManager;
class CPUThread;
class EventManager;
class ModuleManager;
struct VFS;

struct EmuInfo
{
private:
	friend class Emulator;

	u32 m_tls_addr = 0;
	u32 m_tls_filesz = 0;
	u32 m_tls_memsz = 0;
	u32 m_sdk_version = 0x360001;
	u32 m_malloc_pagesize = 0x100000;
	u32 m_primary_stacksize = 0x100000;
	s32 m_primary_prio = 0x50;

public:
	EmuInfo()
	{
	}
};

class Emulator final
{
	EmuCallbacks m_cb;

	enum Mode
	{
		DisAsm,
		InterpreterDisAsm,
		Interpreter,
	};
		
	volatile u32 m_status;
	uint m_mode;

	std::atomic<u64> m_pause_start_time; // set when paused
	std::atomic<u64> m_pause_amend_time; // increased when resumed

	u32 m_rsx_callback;
	u32 m_cpu_thr_stop;

	std::vector<u64> m_break_points;
	std::vector<u64> m_marked_points;

	std::mutex m_core_mutex;

	std::unique_ptr<CPUThreadManager> m_thread_manager;
	std::unique_ptr<PadManager>       m_pad_manager;
	std::unique_ptr<KeyboardManager>  m_keyboard_manager;
	std::unique_ptr<MouseManager>     m_mouse_manager;
	std::unique_ptr<GSManager>        m_gs_manager;
	std::unique_ptr<AudioManager>     m_audio_manager;
	std::unique_ptr<CallbackManager>  m_callback_manager;
	std::unique_ptr<EventManager>     m_event_manager;
	std::unique_ptr<ModuleManager>    m_module_manager;
	std::unique_ptr<VFS>              m_vfs;

	EmuInfo m_info;
	loader::loader m_loader;

	std::string m_path;
	std::string m_elf_path;
	std::string m_title_id;
	std::string m_title;

public:
	Emulator();

	void SetCallbacks(EmuCallbacks&& cb)
	{
		m_cb = std::move(cb);
	}

	const auto& GetCallbacks() const
	{
		return m_cb;
	}

	void SendDbgCommand(DbgCommand cmd, class CPUThread* thread = nullptr)
	{
		if (m_cb.send_dbg_command) m_cb.send_dbg_command(cmd, thread);
	}

	// Returns a future object associated with the result of the function called from the GUI thread
	template<typename F>
	std::future<void> CallAfter(F&& func) const
	{
		// Make "shared" promise to workaround std::function limitation
		auto spr = std::make_shared<std::promise<void>>();

		// Get future
		std::future<void> future = spr->get_future();

		// Run asynchronously in GUI thread
		m_cb.call_after([spr = std::move(spr), task = std::forward<F>(func)]()
		{
			try
			{
				task();
				spr->set_value();
			}
			catch (...)
			{
				spr->set_exception(std::current_exception());
			}
		});

		return future;
	}

	/** Set emulator mode to running unconditionnaly.
	 * Required to execute various part (PPUInterpreter, memory manager...) outside of rpcs3.
	 */
	void SetTestMode()
	{
		m_status = Running;
	}

	void Init();
	void SetPath(const std::string& path, const std::string& elf_path = "");
	void SetTitleID(const std::string& id);
	void SetTitle(const std::string& title);
	void CreateConfig(const std::string& name);

	const std::string& GetPath() const
	{
		return m_elf_path;
	}

	const std::string& GetTitleID() const
	{
		return m_title_id;
	}

	const std::string& GetTitle() const
	{
		return m_title;
	}

	u64 GetPauseTime()
	{
		return m_pause_amend_time;
	}

	std::mutex&       GetCoreMutex()       { return m_core_mutex; }
	CPUThreadManager& GetCPU()             { return *m_thread_manager; }
	PadManager&       GetPadManager()      { return *m_pad_manager; }
	KeyboardManager&  GetKeyboardManager() { return *m_keyboard_manager; }
	MouseManager&     GetMouseManager()    { return *m_mouse_manager; }
	GSManager&        GetGSManager()       { return *m_gs_manager; }
	AudioManager&     GetAudioManager()    { return *m_audio_manager; }
	CallbackManager&  GetCallbackManager() { return *m_callback_manager; }
	VFS&              GetVFS()             { return *m_vfs; }
	std::vector<u64>& GetBreakPoints()     { return m_break_points; }
	std::vector<u64>& GetMarkedPoints()    { return m_marked_points; }
	EventManager&     GetEventManager()    { return *m_event_manager; }
	ModuleManager&    GetModuleManager()   { return *m_module_manager; }

	void ResetInfo()
	{
		m_info = {};
	}

	void SetTLSData(u32 addr, u32 filesz, u32 memsz)
	{
		m_info.m_tls_addr = addr;
		m_info.m_tls_filesz = filesz;
		m_info.m_tls_memsz = memsz;
	}

	void SetParams(u32 sdk_ver, u32 malloc_pagesz, u32 stacksz, s32 prio)
	{
		m_info.m_sdk_version = sdk_ver;
		m_info.m_malloc_pagesize = malloc_pagesz;
		m_info.m_primary_stacksize = stacksz;
		m_info.m_primary_prio = prio;
	}

	void SetRSXCallback(u32 addr)
	{
		m_rsx_callback = addr;
	}

	void SetCPUThreadStop(u32 addr)
	{
		m_cpu_thr_stop = addr;
	}

	u32 GetTLSAddr() const { return m_info.m_tls_addr; }
	u32 GetTLSFilesz() const { return m_info.m_tls_filesz; }
	u32 GetTLSMemsz() const { return m_info.m_tls_memsz; }

	u32 GetMallocPageSize() { return m_info.m_malloc_pagesize; }
	u32 GetSDKVersion() { return m_info.m_sdk_version; }
	u32 GetPrimaryStackSize() { return m_info.m_primary_stacksize; }
	s32 GetPrimaryPrio() { return m_info.m_primary_prio; }

	u32 GetRSXCallback() const { return m_rsx_callback; }
	u32 GetCPUThreadStop() const { return m_cpu_thr_stop; }

	bool BootGame(const std::string& path, bool direct = false);

	void Load();
	void Run();
	bool Pause();
	void Resume();
	void Stop();

	void SavePoints(const std::string& path);
	bool LoadPoints(const std::string& path);

	force_inline bool IsRunning() const { return m_status == Running; }
	force_inline bool IsPaused()  const { return m_status == Paused; }
	force_inline bool IsStopped() const { return m_status == Stopped; }
	force_inline bool IsReady()   const { return m_status == Ready; }
};

extern Emulator Emu;

using lv2_lock_t = std::unique_lock<std::mutex>;

inline bool check_lv2_lock(lv2_lock_t& lv2_lock)
{
	return lv2_lock.owns_lock() && lv2_lock.mutex() == &Emu.GetCoreMutex();
}

#define LV2_LOCK lv2_lock_t lv2_lock(Emu.GetCoreMutex())
#define LV2_DEFER_LOCK lv2_lock_t lv2_lock
#define CHECK_LV2_LOCK(x) if (!check_lv2_lock(x)) throw EXCEPTION("lv2_lock is invalid or not locked")
#define CHECK_EMU_STATUS if (Emu.IsStopped()) throw EmulationStopped{}
