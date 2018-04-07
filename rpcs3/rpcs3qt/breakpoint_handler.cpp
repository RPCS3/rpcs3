#include "breakpoint_handler.h"

// PPU Thread functions concerning breakpoints.
extern void ppu_set_breakpoint(u32 addr);
extern void ppu_remove_breakpoint(u32 addr);

breakpoint_handler::breakpoint_handler(QObject* parent) : QObject(parent), m_breakpoints(),
	m_gameid("default"), m_breakpoint_settings(this), cpu()
{
	// Annoying hack since lambdas with captures aren't passed as function pointers--- prevents access violation later if I tried to pass it to ppu_set_breakpoint.
	// I'll probably try and revise this later, but I wanted to get something working for now.
	EmuCallbacks callbacks = Emu.GetCallbacks();
	callbacks.on_ppu_breakpoint_triggered = [this](const ppu_thread& ppu) {
		Q_EMIT BreakpointTriggered(&ppu, ppu.cia);
	};
	Emu.SetCallbacks(std::move(callbacks));
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
	if (!(val & (u32) breakpoint_types::exec) && (flags & (u32) breakpoint_types::exec))
	{
		ppu_set_breakpoint(addr);
		m_breakpoints[m_gameid][addr] = {val | (u32) breakpoint_types::exec, name};
		if (!transient)
		{
			m_breakpoint_settings.SetBreakpoint(m_gameid, addr, m_breakpoints[m_gameid][addr]);
		}
	}

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
	if ((val & (u32) breakpoint_types::exec) && (flags & (u32) breakpoint_types::exec))
	{
		ppu_remove_breakpoint(addr);
		m_breakpoints[m_gameid][addr].flags = val - (u32) breakpoint_types::exec;
		HandleRemove(addr);
	}
	// TODO handle other breakpoint types
}

void breakpoint_handler::test()
{

	auto cpu = this->cpu.lock();
	if (cpu)
	{
		u32 addr = 0x8abe9c; // TOCS 2
		auto native_handle = (thread_handle)cpu->get()->get_native_handle();
		auto handle = hw_breakpoint_manager::set(native_handle, hw_breakpoint_type::read_write, hw_breakpoint_size::size_4, (u64)vm::g_base_addr + addr, nullptr, [this](const cpu_thread* cpu, hw_breakpoint& breakpoint)
		{
			emit BreakpointTriggered(cpu, breakpoint.get_address());
		});
	}
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

	for (u32 addr : m_breakpoints[m_gameid].keys())
	{
		// todo: add databreakpoints
		ppu_set_breakpoint(addr);
	}
}

void breakpoint_handler::UpdateCPUThread(std::weak_ptr<cpu_thread> cpu)
{
	this->cpu = cpu;
}
