#pragma once

#include "Loader/Loader.h"

enum Status
{
	Running,
	Paused,
	Stopped,
	Ready,
};

class CPUThreadManager;
class PadManager;
class KeyboardManager;
class MouseManager;
class ID_manager;
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
	u32 tls_addr;
	u32 tls_filesz;
	u32 tls_memsz;

	sys_process_param_info proc_param;

public:
	EmuInfo() { Reset(); }

	sys_process_param_info& GetProcParam() { return proc_param; }

	void Reset()
	{
		SetTLSData(0, 0, 0);
		memset(&proc_param, 0, sizeof(sys_process_param_info));

		proc_param.malloc_pagesize = be_t<u32>::make(0x100000);
		proc_param.sdk_version = be_t<u32>::make(0x360001);
		proc_param.primary_stacksize = be_t<u32>::make(0x100000);
		proc_param.primary_prio = be_t<s32>::make(0x50);
	}

	void SetTLSData(u32 addr, u32 filesz, u32 memsz)
	{
		tls_addr = addr;
		tls_filesz = filesz;
		tls_memsz = memsz;
	}

	u32 GetTLSAddr() const { return tls_addr; }
	u32 GetTLSFilesz() const { return tls_filesz; }
	u32 GetTLSMemsz() const { return tls_memsz; }
};

class Emulator
{
	enum Mode
	{
		DisAsm,
		InterpreterDisAsm,
		Interpreter,
	};
		
	volatile uint m_status;
	uint m_mode;

	u32 m_rsx_callback;
	u32 m_cpu_thr_stop;

	std::vector<u64> m_break_points;
	std::vector<u64> m_marked_points;

	std::mutex m_core_mutex;

	CPUThreadManager* m_thread_manager;
	PadManager* m_pad_manager;
	KeyboardManager* m_keyboard_manager;
	MouseManager* m_mouse_manager;
	ID_manager* m_id_manager;
	GSManager* m_gs_manager;
	AudioManager* m_audio_manager;
	CallbackManager* m_callback_manager;
	EventManager* m_event_manager;
	ModuleManager* m_module_manager;
	VFS* m_vfs;

	EmuInfo m_info;
	loader::loader m_loader;

public:
	std::string m_path;
	std::string m_elf_path;
	std::string m_emu_path;
	std::string m_title_id;
	std::string m_title;

	Emulator();
	~Emulator();

	void Init();
	void SetPath(const std::string& path, const std::string& elf_path = "");
	void SetTitleID(const std::string& id);
	void SetTitle(const std::string& title);

	std::string GetPath() const
	{
		return m_elf_path;
	}

	std::string GetEmulatorPath() const
	{
		return m_emu_path;
	}

	const std::string& GetTitleID() const
	{
		return m_title_id;
	}

	const std::string& GetTitle() const
	{
		return m_title;
	}

	void SetEmulatorPath(const std::string& path)
	{
		m_emu_path = path;
	}

	std::mutex&       GetCoreMutex()       { return m_core_mutex; }

	CPUThreadManager& GetCPU()             { return *m_thread_manager; }
	PadManager&       GetPadManager()      { return *m_pad_manager; }
	KeyboardManager&  GetKeyboardManager() { return *m_keyboard_manager; }
	MouseManager&     GetMouseManager()    { return *m_mouse_manager; }
	ID_manager&       GetIdManager()       { return *m_id_manager; }
	GSManager&        GetGSManager()       { return *m_gs_manager; }
	AudioManager&     GetAudioManager()    { return *m_audio_manager; }
	CallbackManager&  GetCallbackManager() { return *m_callback_manager; }
	VFS&              GetVFS()             { return *m_vfs; }
	std::vector<u64>& GetBreakPoints()     { return m_break_points; }
	std::vector<u64>& GetMarkedPoints()    { return m_marked_points; }
	EventManager&     GetEventManager()    { return *m_event_manager; }
	ModuleManager&    GetModuleManager()   { return *m_module_manager; }

	void SetTLSData(u32 addr, u32 filesz, u32 memsz)
	{
		m_info.SetTLSData(addr, filesz, memsz);
	}

	void SetRSXCallback(u32 addr)
	{
		m_rsx_callback = addr;
	}

	void SetCPUThreadStop(u32 addr)
	{
		m_cpu_thr_stop = addr;
	}

	EmuInfo& GetInfo() { return m_info; }

	u32 GetTLSAddr() const { return m_info.GetTLSAddr(); }
	u32 GetTLSFilesz() const { return m_info.GetTLSFilesz(); }
	u32 GetTLSMemsz() const { return m_info.GetTLSMemsz(); }

	u32 GetMallocPageSize() { return m_info.GetProcParam().malloc_pagesize; }
	u32 GetSDKVersion() { return m_info.GetProcParam().sdk_version; }

	u32 GetRSXCallback() const { return m_rsx_callback; }
	u32 GetCPUThreadStop() const { return m_cpu_thr_stop; }

	void CheckStatus();
	bool BootGame(const std::string& path, bool direct = false);

	void Load();
	void Run();
	void Pause();
	void Resume();
	void Stop();

	void SavePoints(const std::string& path);
	void LoadPoints(const std::string& path);

	force_inline bool IsRunning() const { return m_status == Running; }
	force_inline bool IsPaused()  const { return m_status == Paused; }
	force_inline bool IsStopped() const { return m_status == Stopped; }
	force_inline bool IsReady()   const { return m_status == Ready; }
};

using lv2_lock_type = std::unique_lock<std::mutex>;

#define LV2_LOCK lv2_lock_type lv2_lock(Emu.GetCoreMutex())
#define CHECK_LV2_LOCK(x) assert((x).owns_lock() && (x).mutex() == &Emu.GetCoreMutex())

extern Emulator Emu;

typedef void(*CallAfterCbType)(std::function<void()> func);

void CallAfter(std::function<void()> func);

void SetCallAfterCallback(CallAfterCbType cb);
