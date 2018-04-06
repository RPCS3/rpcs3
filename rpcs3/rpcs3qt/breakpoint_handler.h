#pragma once
#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "Utilities/hardware_breakpoint/hardware_breakpoint_manager.h"

#include "breakpoint_settings.h"

#include <QMap>

// Should remove this and just use hardware_breakpoint since in reality, it seems like the flag combinations are more limited than I thought.
// It'll make the code a lot cleaner.
// I was aiming to support some idiot having a ppu breakpoint and a data breakpoint at the same location.
// But, let's be real, that really isn't necessary in any universe since, in retrospect, code and data are separated in ps3 elf files  
// which is why that PPU LLVM was so profitable while SPU LLVM isn't to my knowledge
// (how would you do LLVM with self modifying code. I'll believe it when it happens because otherwise it sounds too good to be true)

// Also, given how differently these breakpoints are setup, sans a fun merge of classes, I think it'll be best to have an addbreakpoint and 
// an adddatabreakpoint function. Qt slots won't mind since you can just use lambdas to respond to the (to be made) breakpoint dialog.
// ... which further makes breakpoint_types unnecessary :shrug:
enum class breakpoint_types
{
	read = 0x1,
	write = 0x2,
	exec = 0x4,
};

/*
* This class acts as a layer between the UI and Emu for breakpoints.
* You may ask why not make a static class, like yours. Answer is simple. Qt signals/slots are object base.  So, you'd have to use a singleton.
* And, adding a singleton to a codebase is not something I'm willing to do.
* I've seen what happens when you have too many singletons in a codebase when working corporate.
* It's bad.
* Anyhow, I tried to write this as generally as possible, just so that it could be merged with anything.  Seems like it is definitely doable.
* I got most of it done.  I got greedy and implemented thread switching.
*/
class breakpoint_handler : public QObject
{
	Q_OBJECT
Q_SIGNALS:
	void BreakpointTriggered(const cpu_thread* thrd, u32 addr);
public:
	explicit breakpoint_handler(QObject* parent = nullptr);

	/**
	* Returns true if breakpoint exists at location (w/ game_id) having said flags, otherwise returns 0.
	* If 0x0 is specified for flags, simply check if breakpoint exists at location.
	*/
	bool HasBreakpoint(u32 addr, u32 flags = 0x0) const;

	/**
	* Adds a breakpoint at specified address. If breakpoint exists at same address, the flags are unioned.
	* A transient breakpoint is one that does not get saved to disc.  This would happen when breakpoints are used internally, ie. step over
	* Size is ignored if using exec breakpoint.
	*/
	void AddBreakpoint(u32 addr, u32 flags, const QString& name, bool transient = false);

	/*
	* Changes the specified breakpoint's name. 
	*/
	void RenameBreakpoint(u32 addr, const QString& newName);

	/**
	* Removes the breakpoint at address specified game_id.
	*/
	void RemoveBreakpoint(u32 addr, u32 flags);

	/**
	* Gets the current breakpoints.  Namely, m_breakpoints[m_gameid]
	*/
	QMap<u32, breakpoint_data> GetCurrentBreakpoints()
	{
		return m_breakpoints[m_gameid];
	}

	/**
	* Sets gameid so that I don't need to spam that same argument everywhere.
	*/
	void UpdateGameID();

	/*
	* Ugh. For hardware breakpoints, I need access to the thread.  So, pass it in here.
	*/
	void UpdateCPUThread(std::weak_ptr<cpu_thread> cpu);

	/*
	* delete later.  It's just here so I can test the code before making a proper UI. (proper UI is always last :D)
	*/
	void test();
private:
	void HandleRemove(u32 addr);
	QMap<QString, QMap<u32, breakpoint_data>> m_breakpoints; //! GameID -> Addr --> Breakpoint Type(s) (read/write/exec)
	QString m_gameid; // ID of current game being used for breakpoints.
	breakpoint_settings m_breakpoint_settings; // Used to read and write settings to disc.
	std::weak_ptr<cpu_thread> cpu; // needed for hardware breakpoints.
	// Does the thread itself really matter because if so, it'll be a PAIN.  For one, there's no callback for threadcreated in RPCS3 (easy).. 
	// Though, adding a callback to a PPU function.  Neko will be thrilled/s. 
	// Plus, some games make multiple threads with same name, so name isn't reliable. Can you really go by ID? I don't think you can, safely, at least.
	// That's just asking for race conditions.
};
