#pragma once
#include "stdafx.h"
#include "Emu/Cell/PPUThread.h"
#include <set>

enum breakpoint_types
{
	read = 0x1,
	write = 0x2,
	exec = 0x4,
};

/*
* This class acts as a layer between the UI and Emu for breakpoints.
*/
class breakpoint_handler
{

public:
	breakpoint_handler();
	~breakpoint_handler();

	/**
	* Returns true iff breakpoint exists at loc.
	* TODO: Add arg for flags, gameid.
	*/
	bool HasBreakpoint(u32 loc) const;

	/**
	* Returns true if added successfully. TODO: flags
	*/
	bool AddBreakpoint(u32 loc);


	/**
	* Returns true if removed breakpoint at loc successfully.
	*/
	bool RemoveBreakpoint(u32 loc);

private:
	// TODO : generalize to hold multiple games and handle flags.Probably do : std::map<std::string, std::set<breakpoint>>.
	// May also do std::map<loc, breakpoint> where breakpoint has gameid depending on how map is used most.
	std::set<u32> m_breakpoints; //! Holds all breakpoints.
};