#pragma once
#include "Gui/MainFrame.h"

template<typename T> T min(const T a, const T b) { return a < b ? a : b; }
template<typename T> T max(const T a, const T b) { return a > b ? a : b; }

//#define re(val) MemoryBase::Reverse(val)
#define re64(val) MemoryBase::Reverse64(val)
#define re32(val) MemoryBase::Reverse32(val)
#define re16(val) MemoryBase::Reverse16(val)

template<typename T> T re(const T val) { T res; se_t<T>::func(res, val); return res; }
template<typename T1, typename T2> void re(T1& dst, const T2 val) { se_t<T1>::func(dst, val); }

extern const wxEventType wxEVT_DBG_COMMAND;

enum DbgCommand
{
	DID_FIRST_COMMAND = 0x500,

	DID_START_EMU,
	DID_STARTED_EMU,
	DID_STOP_EMU,
	DID_STOPPED_EMU,
	DID_PAUSE_EMU,
	DID_PAUSED_EMU,
	DID_RESUME_EMU,
	DID_RESUMED_EMU,
	DID_READY_EMU,
	DID_CREATE_THREAD,
	DID_CREATED_THREAD,
	DID_REMOVE_THREAD,
	DID_REMOVED_THREAD,
	DID_RENAME_THREAD,
	DID_RENAMED_THREAD,
	DID_START_THREAD,
	DID_STARTED_THREAD,
	DID_STOP_THREAD,
	DID_STOPED_THREAD,
	DID_PAUSE_THREAD,
	DID_PAUSED_THREAD,
	DID_RESUME_THREAD,
	DID_RESUMED_THREAD,
	DID_EXEC_THREAD,
	DID_REGISTRED_CALLBACK,
	DID_UNREGISTRED_CALLBACK,
	DID_EXIT_THR_SYSCALL,

	DID_LAST_COMMAND,
};

class Rpcs3App : public wxApp
{
public:
	MainFrame* m_MainFrame;

	virtual bool OnInit();       // RPCS3's entry point
	virtual void OnArguments();  // Handle arguments: Rpcs3App::argc, Rpcs3App::argv
	virtual void Exit();

	Rpcs3App();

	void SendDbgCommand(DbgCommand id, CPUThread* thr=nullptr);
};

DECLARE_APP(Rpcs3App)

//extern CPUThread& GetCPU(const u8 core);

extern Rpcs3App* TheApp;
static const u64 PS3_CLK = 3200000000;
