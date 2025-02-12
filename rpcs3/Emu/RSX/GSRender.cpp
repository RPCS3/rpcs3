#include "stdafx.h"

#include "Emu/System.h"
#include "GSRender.h"

GSRender::GSRender(utils::serial* ar) noexcept : rsx::thread(ar)
{
	if (auto gs_frame = Emu.GetCallbacks().get_gs_frame())
	{
		m_frame = gs_frame.release();
	}
	else
	{
		m_frame = nullptr;
	}
}

GSRender::~GSRender()
{
	m_context = nullptr;

	if (m_frame && !m_continuous_mode)
	{
		m_frame->close();
	}
}

void GSRender::on_init_thread()
{
	if (m_frame)
	{
		m_context = m_frame->make_context();
		m_frame->set_current(m_context);
	}
}

void GSRender::on_exit()
{
	rsx::thread::on_exit();

	if (m_frame)
	{
		if (!m_continuous_mode)
		{
			m_frame->hide();
		}
		m_frame->delete_context(m_context);
		m_context = nullptr;
	}
}

void GSRender::flip(const rsx::display_flip_info_t&)
{
	if (m_frame)
	{
		m_frame->flip(m_context);
	}
}

f64 GSRender::get_display_refresh_rate() const
{
	if (m_frame)
	{
		return m_frame->client_display_rate();
	}

	// Minimum
	return 20.;
}
