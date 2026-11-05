#include "stdafx.h"
#include "RSXRenderBackend.h"

#include "Emu/RSX/Core/RSXDisplayOutput.h"
#include "Emu/RSX/GSFrameBase.h"
#include "Emu/RSX/Host/RSXDMAWriter.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/system_config.h"

#include "Emu/RSX/NV47/HW/context_accessors.define.h"

namespace rsx
{
	render_backend::render_backend(context* ctx, display_output* display)
		: m_ctx(ctx)
		, m_frame(display ? display->frame() : nullptr)
		, m_vsync_mode(g_cfg.video.vsync)
		, m_profiler(RSX(ctx)->profiler())
	{
	}

	render_backend::~render_backend() = default;

	void render_backend::bind_context(context* ctx)
	{
		ensure(ctx);

		if (ctx == m_ctx)
		{
			return;
		}

		on_unbind_context(m_ctx);
		m_ctx = ctx;
		on_bind_context(ctx);
	}

	void render_backend::create_frame_context()
	{
		if (m_frame)
		{
			m_context = m_frame->make_context();
			m_frame->set_current(m_context);
		}
	}

	void render_backend::destroy_frame_context()
	{
		if (m_frame)
		{
			m_frame->delete_context(m_context);
			m_context = nullptr;
		}
	}
}
