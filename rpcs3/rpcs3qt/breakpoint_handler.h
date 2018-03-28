#pragma once
#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"

#include "breakpoint_settings.h"

#include <QMap>

enum class breakpoint_types : u32
{
	read = 0x1,
	write = 0x2,
	exec = 0x4,
};

/*
* This class acts as a layer between the UI and Emu for breakpoints.
*/
class breakpoint_handler : QObject
{
	Q_OBJECT
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
private:
	void HandleRemove(u32 addr);

	QMap<QString, QMap<u32, breakpoint_data>> m_breakpoints; //! GameID -> Addr --> Breakpoint Type(s) (read/write/exec)
	QString m_gameid; // ID of current game being used for breakpoints.
	breakpoint_settings m_breakpoint_settings; // Used to read and write settings to disc.
};
