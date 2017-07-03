#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "GSRender.h"

draw_context_t GSFrameBase::new_context()
{
	if (void* context = make_context())
	{
		return std::shared_ptr<void>(context, [this](void* ctxt) { delete_context(ctxt); });
	}

	return nullptr;
}

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
		m_context = m_frame->new_context();
		m_frame->set_current(m_context);
	}
}

void GSRender::flip(int buffer)
{
	if (m_frame)
	{
		m_frame->flip(m_context);
	}
}
