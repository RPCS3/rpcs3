#include "breakpoint_handler.h"

extern void ppu_breakpoint(u32 loc, bool isAdding);

breakpoint_handler::breakpoint_handler() :m_breakpoints()
{

}

breakpoint_handler::~breakpoint_handler()
{

}

bool breakpoint_handler::HasBreakpoint(u32 loc) const
{
	return m_breakpoints.find(loc) != m_breakpoints.end();
}

bool breakpoint_handler::AddBreakpoint(u32 loc)
{
	m_breakpoints.insert(loc);
	ppu_breakpoint(loc, true);
	return true;
}

bool breakpoint_handler::RemoveBreakpoint(u32 loc)
{
	m_breakpoints.erase(loc);
	ppu_breakpoint(loc, false);
	return true;
}
