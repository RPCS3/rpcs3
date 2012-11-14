#pragma once

#include "Gui/MemoryViewer.h"
#include "Emu/Cell/PPCThreadManager.h"
#include "Emu/Io/Pad.h"
#include "Emu/DbgConsole.h"
#include "Emu/GS/GSManager.h"
#include "Loader/Loader.h"

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

class Emulator
{
	enum Mode
	{
		DisAsm,
		InterpreterDisAsm,
		Interpreter,
	};

	Status m_status;
	uint m_mode;

	u32 m_rsx_callback;
	MemoryViewerPanel* m_memory_viewer;
	//ArrayF<CPUThread> m_cpu_threads;

	PPCThreadManager m_thread_manager;
	PadManager m_pad_manager;
	IdManager m_id_manager;
	DbgConsole* m_dbg_console;
	GSManager m_gs_manager;

	EmuInfo m_info;

public:
	wxString m_path;
	bool IsSelf;

	Emulator();

	void Init();
	virtual void SetSelf(const wxString& path);
	virtual void SetElf(const wxString& path);

	PPCThreadManager&	GetCPU()		{ return m_thread_manager; }
	PadManager&			GetPadManager()	{ return m_pad_manager; }
	IdManager&			GetIdManager()	{ return m_id_manager; }
	DbgConsole&			GetDbgCon()		{ return *m_dbg_console; }
	GSManager&			GetGSManager()	{ return m_gs_manager; }

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

	void CheckStatus();

	virtual void Run();
	virtual void Pause();
	virtual void Resume();
	virtual void Stop();

	virtual bool IsRunned() const { return m_status == Runned; }
	virtual bool IsPaused() const { return m_status == Paused; }
	virtual bool IsStopped() const { return m_status == Stopped; }
};

extern Emulator Emu;