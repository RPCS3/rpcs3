#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/CPU/CPUThread.h"
#include "sleep_queue.h"

sleep_queue_entry_t::sleep_queue_entry_t(CPUThread& cpu, sleep_queue_t& queue)
	: m_queue(queue)
	, m_thread(cpu)
{
	m_queue.emplace_back(cpu.shared_from_this());

	m_thread.Sleep();
}

sleep_queue_entry_t::~sleep_queue_entry_t() noexcept(false)
{
	m_thread.Awake();

	if (m_queue.front().get() == &m_thread)
	{
		m_queue.pop_front();
		return;
	}

	if (m_queue.back().get() == &m_thread)
	{
		m_queue.pop_back();
		return;
	}

	for (auto it = m_queue.begin(); it != m_queue.end(); it++)
	{
		if (it->get() == &m_thread)
		{
			m_queue.erase(it);
			return;
		}
	}

	if (!std::uncaught_exception())
	{
		throw EXCEPTION("Thread not found");
	}
}
