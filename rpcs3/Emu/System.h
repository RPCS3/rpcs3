#pragma once

#include <atomic>
#include "Gui/MemoryViewer.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Io/Pad.h"
#include "Emu/Io/Keyboard.h"
#include "Emu/Io/Mouse.h"
#include "Emu/GS/GSManager.h"
#include "Emu/Audio/AudioManager.h"
#include "Emu/FS/VFS.h"
#include "Emu/DbgConsole.h"
#include "Loader/Loader.h"
#include "SysCalls/Callback.h"

class EventManager;
extern void UnloadModules();

struct EmuInfo
{
private:
	u64 tls_addr;
	u64 tls_filesz;
	u64 tls_memsz;

	sys_process_param_info proc_param;

public:
	EmuInfo() { Reset(); }

	sys_process_param_info& GetProcParam() { return proc_param; }

	void Reset()
	{
		SetTLSData(0, 0, 0);
		memset(&proc_param, 0, sizeof(sys_process_param_info));

		proc_param.malloc_pagesize = 0x100000;
		proc_param.sdk_version = 0x360001;
		//TODO
	}

	void SetTLSData(const u64 addr, const u64 filesz, const u64 memsz)
	{
		tls_addr = addr;
		tls_filesz = filesz;
		tls_memsz = memsz;
	}

	u64 GetTLSAddr() const { return tls_addr; }
	u64 GetTLSFilesz() const { return tls_filesz; }
	u64 GetTLSMemsz() const { return tls_memsz; }
};

class ModuleInitializer
{
public:
	ModuleInitializer();

	virtual void Init() = 0;
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
	u32 m_ppu_thr_exit;
	MemoryViewerPanel* m_memory_viewer;
	//ArrayF<CPUThread> m_cpu_threads;
	std::vector<std::unique_ptr<ModuleInitializer>> m_modules_init;

	std::vector<u64> m_break_points;
	std::vector<u64> m_marked_points;

	CPUThreadManager m_thread_manager;
	PadManager m_pad_manager;
	KeyboardManager m_keyboard_manager;
	MouseManager m_mouse_manager;
	IdManager m_id_manager;
	DbgConsole* m_dbg_console;
	GSManager m_gs_manager;
	AudioManager m_audio_manager;
	CallbackManager m_callback_manager;
	CPUThread* m_ppu_callback_thr;
	std::unique_ptr<EventManager> m_event_manager;

	VFS m_vfs;

	EmuInfo m_info;

public:
	std::string m_path;
	std::string m_elf_path;
	std::string m_title_id;

	Emulator();

	void Init();
	void SetPath(const std::string& path, const std::string& elf_path = "");
	void SetTitleID(const std::string& id);

	CPUThreadManager& GetCPU()             { return m_thread_manager; }
	PadManager&       GetPadManager()      { return m_pad_manager; }
	KeyboardManager&  GetKeyboardManager() { return m_keyboard_manager; }
	MouseManager&     GetMouseManager()    { return m_mouse_manager; }
	IdManager&        GetIdManager()       { return m_id_manager; }
	DbgConsole&       GetDbgCon()          { return *m_dbg_console; }
	GSManager&        GetGSManager()       { return m_gs_manager; }
	AudioManager&     GetAudioManager()    { return m_audio_manager; }
	CallbackManager&  GetCallbackManager() { return m_callback_manager; }
	VFS&              GetVFS()             { return m_vfs; }
	std::vector<u64>& GetBreakPoints()     { return m_break_points; }
	std::vector<u64>& GetMarkedPoints()    { return m_marked_points; }
	CPUThread&        GetCallbackThread()  { return *m_ppu_callback_thr; }
	EventManager&     GetEventManager()    { return *m_event_manager; }
	
	void AddModuleInit(std::unique_ptr<ModuleInitializer> m)
	{
		m_modules_init.push_back(std::move(m));
	}

	void SetTLSData(const u64 addr, const u64 filesz, const u64 memsz)
	{
		m_info.SetTLSData(addr, filesz, memsz);
	}

	EmuInfo& GetInfo() { return m_info; }

	u64 GetTLSAddr() const { return m_info.GetTLSAddr(); }
	u64 GetTLSFilesz() const { return m_info.GetTLSFilesz(); }
	u64 GetTLSMemsz() const { return m_info.GetTLSMemsz(); }

	u32 GetMallocPageSize() { return m_info.GetProcParam().malloc_pagesize; }

	u32 GetRSXCallback() const { return m_rsx_callback; }
	u32 GetPPUThreadExit() const { return m_ppu_thr_exit; }

	void CheckStatus();
	bool BootGame(const std::string& path);

	void Load();
	void Run();
	void Pause();
	void Resume();
	void Stop();

	void SavePoints(const std::string& path);
	void LoadPoints(const std::string& path);

	__forceinline bool IsRunning() const { return m_status == Running; }
	__forceinline bool IsPaused()  const { return m_status == Paused; }
	__forceinline bool IsStopped() const { return m_status == Stopped; }
	__forceinline bool IsReady()   const { return m_status == Ready; }
};

extern Emulator Emu;
