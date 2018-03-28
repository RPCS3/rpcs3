#include "breakpoint_handler.h"

extern void ppu_breakpoint(u32 loc, bool isAdding);

breakpoint_handler::breakpoint_handler(QObject* parent) : QObject(parent), m_breakpoints(),
	m_gameid("default"), m_breakpoint_settings(this)
{
}

bool breakpoint_handler::HasBreakpoint(u32 addr, u32 flags) const
{
	u32 val = m_breakpoints[m_gameid].value(addr, {}).flags;
	return val && ((val & flags) == flags);
}

void breakpoint_handler::AddBreakpoint(u32 addr, u32 flags, const QString& name, bool transient)
{
	if (HasBreakpoint(addr, flags)) return;
	u32 val = m_breakpoints[m_gameid].value(addr, {}).flags;
	if (!(val & static_cast<u32>(breakpoint_types::exec)) && (flags & static_cast<u32>(breakpoint_types::exec)))
	{
		ppu_breakpoint(addr, true);
		m_breakpoints[m_gameid][addr] = {val | static_cast<u32>(breakpoint_types::exec), name};
		if (!transient)
		{
			m_breakpoint_settings.SetBreakpoint(m_gameid, addr, m_breakpoints[m_gameid][addr]);
		}
	}
	// TODO: Add other breakpoint types.
}

void breakpoint_handler::RenameBreakpoint(u32 addr, const QString& newName)
{
	if (HasBreakpoint(addr))
	{
		m_breakpoints[m_gameid][addr].name = newName;
		m_breakpoint_settings.SetBreakpoint(m_gameid, addr, m_breakpoints[m_gameid][addr]);
	}
}

void breakpoint_handler::RemoveBreakpoint(u32 addr, u32 flags)
{
	u32 val = m_breakpoints[m_gameid].value(addr, {}).flags;
	if ((val & static_cast<u32>(breakpoint_types::exec)) && (flags & static_cast<u32>(breakpoint_types::exec)))
	{
		ppu_breakpoint(addr, false);
		m_breakpoints[m_gameid][addr].flags = val - static_cast<u32>(breakpoint_types::exec);
		HandleRemove(addr);
	}
	// TODO handle other breakpoint types
}

/**
* We need to choose between either removing or setting the value in breakpoint settings.  Do so here.
*/
void breakpoint_handler::HandleRemove(u32 addr)
{
	if (!m_breakpoints[m_gameid][addr].flags)
	{
		m_breakpoints[m_gameid].remove(addr);
		m_breakpoint_settings.RemoveBreakpoint(m_gameid, addr);
	}
	else
	{
		m_breakpoint_settings.SetBreakpoint(m_gameid, addr, m_breakpoints[m_gameid][addr]);
	}
}

void breakpoint_handler::UpdateGameID()
{
	// There's a REALLY annoying edge case where step over fails resulting in a breakpoint just sitting transiently that gets caught on next game boot of same game.
	// Gotta reset to old just to prevent that.
	m_breakpoints = m_breakpoint_settings.ReadBreakpoints();

	m_gameid = QString::fromStdString(Emu.GetTitleID());

	// Doubt this will be triggered, but might as well try to use the title if ID somehow doesn't exist?
	if (m_gameid.length() == 0)
	{
		m_gameid = QString::fromStdString(Emu.GetTitle());
	}

	// If you insist...
	if (m_gameid.length() == 0)
	{
		m_gameid = "default";
	}
}
