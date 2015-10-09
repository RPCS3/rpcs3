#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "GSManager.h"
#include "GSRender.h"

draw_context_t GSFrameBase::new_context()
{
	return std::shared_ptr<void>(make_context(), [this](void* ctxt) { delete_context(ctxt); });
}

GSRender::GSRender(frame_type type) : m_frame(Emu.GetCallbacks().get_gs_frame(type).release())
{
}

GSRender::~GSRender()
{
	m_context = nullptr;

	if (m_frame)
	{
		m_frame->close();
	}
}

void GSRender::oninit()
{
	if (m_frame)
	{
		m_frame->show();
	}
}

void GSRender::oninit_thread()
{
	if (m_frame)
	{
		m_context = m_frame->new_context();
		m_frame->set_current(m_context);
	}
}

void GSRender::close()
{
	if (m_frame && m_frame->shown())
	{
		m_frame->hide();
	}

	if (joinable())
	{
		join();
	}
}

void GSRender::flip(int buffer)
{
	if (m_frame)
		m_frame->flip(m_context);
}


GSLock::GSLock(GSRender& renderer, GSLockType type)
	: m_renderer(renderer)
	, m_type(type)
{
	switch (m_type)
	{
	case GS_LOCK_NOT_WAIT: m_renderer.cs_main.lock(); break;
	case GS_LOCK_WAIT_FLIP: m_renderer.sem_flip.wait(); break;
	}
}

GSLock::~GSLock()
{
	switch (m_type)
	{
	case GS_LOCK_NOT_WAIT: m_renderer.cs_main.unlock(); break;
	case GS_LOCK_WAIT_FLIP: m_renderer.sem_flip.try_post(); break;
	}
}

GSLockCurrent::GSLockCurrent(GSLockType type) : GSLock(Emu.GetGSManager().GetRender(), type)
{
}
