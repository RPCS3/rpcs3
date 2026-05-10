#pragma once

#include "util/types.hpp"
#include "Utilities/bit_set.h"
#include <map>
#include "Utilities/mutex.h"
#include <atomic>

enum class breakpoint_types
{
	bp_read = 0x1,
	bp_write = 0x2,
	bp_exec = 0x4,
	__bitset_enum_max
};

/*
* This class acts as a layer between the UI and Emu for breakpoints.
*/
class breakpoint_handler
{

public:
	breakpoint_handler() = default;
	~breakpoint_handler() = default;

	bool IsBreakOnBPM() const;
	void SetBreakOnBPM(bool break_on_bpm);

	/**
	* Returns true iff breakpoint exists at loc.
	* TODO: Add arg for flags, gameid, and maybe even thread if it should be thread local breakpoint.... breakpoint struct is probably what'll happen
	*/
	bool HasBreakpoint(u32 loc, bs_t<breakpoint_types> type);

	/**
	* Returns true if added successfully. TODO: flags
	*/
	bool AddBreakpoint(u32 loc, bs_t<breakpoint_types> type);

	/**
	* Returns true if removed breakpoint at loc successfully.
	*/
	bool RemoveBreakpoint(u32 loc);

private:
	// TODO : generalize to hold multiple games and handle flags.Probably do : std::map<std::string (gameid), std::set<breakpoint>>.
	// Although, externally, they'll only be accessed by loc (I think) so a map of maps may also do?
	shared_mutex mutex_breakpoints;
	std::atomic<bool> m_empty{true};
	std::map<u32, bs_t<breakpoint_types>> m_breakpoints; //! Holds all breakpoints.
	bool m_break_on_bpm = false;
};

extern breakpoint_handler g_breakpoint_handler;
