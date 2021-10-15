#include "breakpoint_handler.h"

extern bool ppu_breakpoint(u32 loc, bool is_adding);

bool breakpoint_handler::HasBreakpoint(u32 loc) const
{
	return m_breakpoints.contains(loc);
}

bool breakpoint_handler::AddBreakpoint(u32 loc)
{
	if (!ppu_breakpoint(loc, true))
	{
		return false;
	}

	ensure(m_breakpoints.insert(loc).second);
	return true;
}

bool breakpoint_handler::RemoveBreakpoint(u32 loc)
{
	if (m_breakpoints.erase(loc) == 0)
	{
		return false;
	}

	ensure(ppu_breakpoint(loc, false));
	return true;
}
