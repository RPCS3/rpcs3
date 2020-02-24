#pragma once
#include "stdafx.h"
#include <set>

enum class breakpoint_types
{
	bp_read = 0x1,
	bp_write = 0x2,
	bp_exec = 0x4,
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
	* TODO: Add arg for flags, gameid, and maybe even thread if it should be thread local breakpoint.... breakpoint struct is probably what'll happen
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
	// TODO : generalize to hold multiple games and handle flags.Probably do : std::map<std::string (gameid), std::set<breakpoint>>.
	// Although, externally, they'll only be accessed by loc (I think) so a map of maps may also do? 
	std::set<u32> m_breakpoints; //! Holds all breakpoints.
};
