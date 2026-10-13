#include "stdafx.h"

#include "Emu/system_config.h"
#include "GSRender.h"

GSRender::GSRender(utils::serial* ar) noexcept : rsx::thread(ar)
{
	m_frame = m_display_out.frame();
	m_vsync_mode = g_cfg.video.vsync;

	// The renderer is its own host backend until the thread/renderer split (spec 03)
	m_renderer = this;
}

GSRender::~GSRender()
{
	m_context = nullptr;
}

void GSRender::on_init()
{
	if (m_frame)
	{
		m_context = m_frame->make_context();
		m_frame->set_current(m_context);
	}
}

void GSRender::on_shutdown()
{
	if (m_frame)
	{
		m_frame->delete_context(m_context);
		m_context = nullptr;
	}
}

bool GSRender::present_frame(const rsx::display_flip_info_t&)
{
	if (m_frame)
	{
		m_frame->flip(m_context);
	}

	return false;
}

f64 GSRender::get_display_refresh_rate() const
{
	return m_display_out.refresh_rate();
}
