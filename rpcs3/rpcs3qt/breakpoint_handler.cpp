#include "breakpoint_handler.h"

extern bool ppu_breakpoint(u32 loc, bool is_adding);

bool breakpoint_handler::IsBreakOnBPM() const
{
	return m_break_on_bpm;
}

void breakpoint_handler::SetBreakOnBPM(bool break_on_bpm)
{
	m_break_on_bpm = break_on_bpm;
}

bool breakpoint_handler::HasBreakpoint(u32 loc, bs_t<breakpoint_types> type)
{
	std::lock_guard lock(mutex_breakpoints);

	return m_breakpoints.contains(loc) && ((m_breakpoints.at(loc) & type) == type);
}

bool breakpoint_handler::AddBreakpoint(u32 loc, bs_t<breakpoint_types> type)
{
	std::lock_guard lock(mutex_breakpoints);

	if ((type & breakpoint_types::bp_exec) && !ppu_breakpoint(loc, true))
	{
		return false;
	}

	return m_breakpoints.insert({loc, type}).second;
}

bool breakpoint_handler::RemoveBreakpoint(u32 loc)
{
	std::lock_guard lock(mutex_breakpoints);

	bs_t<breakpoint_types> bp_type{};
	if (m_breakpoints.contains(loc))
	{
		bp_type = m_breakpoints.at(loc);
	}

	if (m_breakpoints.erase(loc) == 0)
	{
		return false;
	}

	if (bp_type & breakpoint_types::bp_exec)
	{
		ensure(ppu_breakpoint(loc, false));
	}
	return true;
}
