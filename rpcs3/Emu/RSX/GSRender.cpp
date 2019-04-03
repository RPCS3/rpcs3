#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"

#include "GSRender.h"

GSRender::GSRender()
{
	m_frame = Emu.GetCallbacks().get_gs_frame().release();
}

GSRender::~GSRender()
{
	m_context = nullptr;

	if (m_frame)
	{
		m_frame->hide();
		m_frame->close();
	}
}

void GSRender::on_init_rsx()
{
	if (m_frame)
	{
		m_frame->show();
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
	if (m_frame)
	{
		m_frame->disable_wm_event_queue();
		m_frame->clear_wm_events();
		m_frame->delete_context(m_context);
		m_context = nullptr;
	}

	rsx::thread::on_exit();
}

void GSRender::flip(int buffer, bool emu_flip)
{
	if (m_frame)
	{
		m_frame->flip(m_context);
	}
}
