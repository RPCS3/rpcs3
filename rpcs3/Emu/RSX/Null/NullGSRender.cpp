#include "stdafx.h"
#include "NullGSRender.h"

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

bool NullGSRender::present_frame(const rsx::display_flip_info_t&)
{
	if (m_frame)
	{
		m_frame->flip(m_context);
	}

	return false;
}
