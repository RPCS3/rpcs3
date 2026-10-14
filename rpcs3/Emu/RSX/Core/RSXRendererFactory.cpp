#include "stdafx.h"
#include "RSXRendererFactory.h"

#include "Emu/IdManager.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Null/NullGSRender.h"

#ifndef __APPLE__
#include "Emu/RSX/GL/GLGSRender.h"
#endif

#if defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif

namespace rsx
{
	void create_render_thread(utils::serial* ar)
	{
		g_fxo->init<rsx::thread, named_thread<rsx::thread>>(ar);
	}

	std::unique_ptr<render_backend> make_render_backend(video_renderer renderer, context* ctx, display_output* display)
	{
		switch (renderer)
		{
		case video_renderer::opengl:
#if !defined(__APPLE__)
			return std::make_unique<GLGSRender>(ctx, display);
#else
			[[fallthrough]];
#endif
		case video_renderer::vulkan:
#if defined(HAVE_VULKAN)
			return std::make_unique<VKGSRender>(ctx, display);
#else
			[[fallthrough]];
#endif
		default:
		case video_renderer::null:
			return std::make_unique<NullGSRender>(ctx, display);
		}
	}
}
