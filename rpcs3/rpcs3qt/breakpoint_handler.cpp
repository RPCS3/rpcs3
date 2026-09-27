#include "breakpoint_handler.h"

extern bool ppu_breakpoint(lv2_process* process, u32 loc, bool is_adding);

bool breakpoint_handler::IsBreakOnBPM() const
{
	return m_break_on_bpm;
}

void breakpoint_handler::SetBreakOnBPM(bool break_on_bpm)
{
	m_break_on_bpm = break_on_bpm;
}

bool breakpoint_handler::HasBreakpoint(lv2_process*, u64 unique_process_key, u32 loc, bs_t<breakpoint_types> type)
{
	if (m_empty.load(std::memory_order_acquire))
		return false;

	std::lock_guard lock(mutex_breakpoints);

	const auto it = m_breakpoints.find(unique_process_key);

	return it != m_breakpoints.end() && it->second.contains(loc) && ((it->second.at(loc) & type) == type);
}

bool breakpoint_handler::AddBreakpoint(lv2_process* process, u64 unique_process_key, u32 loc, bs_t<breakpoint_types> type)
{
	std::lock_guard lock(mutex_breakpoints);

	if ((type & breakpoint_types::bp_exec) && !ppu_breakpoint(process, loc, true))
	{
		return false;
	}

	auto& map = m_breakpoints[unique_process_key];

	bool result = map.insert({loc, type}).second;
	
	if (result)
	{
		m_empty.store(false, std::memory_order_release);
	}

	return result;
}

bool breakpoint_handler::RemoveBreakpoint(lv2_process* process, u64 unique_process_key, u32 loc)
{
	if (m_empty.load(std::memory_order_acquire))
		return false;

	std::lock_guard lock(mutex_breakpoints);

	const auto it = m_breakpoints.find(unique_process_key);

	if (it == m_breakpoints.end())
	{
		return false;
	}

	bs_t<breakpoint_types> bp_type{};
	if (it->second.contains(loc))
	{
		bp_type = it->second.at(loc);
	}

	if (it->second.erase(loc) == 0)
	{
		return false;
	}

	if (bp_type & breakpoint_types::bp_exec)
	{
		ensure(ppu_breakpoint(process, loc, false));
	}

	if (it->second.empty())
	{
		m_breakpoints.erase(it);
	}

	if (m_breakpoints.empty())
	{
		m_empty.store(true, std::memory_order_release);
	}

	return true;
}
