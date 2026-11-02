#include "stdafx.h"
#include "NullGSRender.h"

#include "Emu/system_config.h"
#include "Emu/RSX/Core/RSXRendererFactory.h"

NullGSRender::NullGSRender(rsx::context* ctx, rsx::display_output* output) noexcept
	: render_backend(ctx, output)
{
}

void NullGSRender::on_init()
{
	create_frame_context();
}

void NullGSRender::on_shutdown()
{
	destroy_frame_context();
}

std::string NullGSRender::configured_logical_id() const
{
	return (rsx::resolve_render_backend_type(g_cfg.video.renderer) == video_renderer::null)
		? "Null"
		: "";
}

bool NullGSRender::present_frame(const rsx::display_flip_info_t&)
{
	if (m_frame)
	{
		m_frame->flip(m_context);
	}

	return false;
}
