#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "GSRender.h"

// temporarily (u8 value is really CellVideoOutResolutionId, but HLE declarations shouldn't be available for the rest of emu, even indirectly)
extern cfg::map_entry<u8> g_cfg_video_out_resolution;
extern const std::unordered_map<u8, std::pair<int, int>> g_video_out_resolution_map;

draw_context_t GSFrameBase::new_context()
{
	if (void* context = make_context())
	{
		return std::shared_ptr<void>(context, [this](void* ctxt) { delete_context(ctxt); });
	}

	return nullptr;
}

GSRender::GSRender(frame_type type)
{
	const auto size = g_video_out_resolution_map.at(g_cfg_video_out_resolution.get());
	m_frame = Emu.GetCallbacks().get_gs_frame(type, size.first, size.second).release();
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
