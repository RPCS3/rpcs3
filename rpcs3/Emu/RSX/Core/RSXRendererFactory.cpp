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

	video_renderer resolve_render_backend_type(video_renderer renderer)
	{
		switch (renderer)
		{
		case video_renderer::opengl:
#if !defined(__APPLE__)
			return video_renderer::opengl;
#else
			[[fallthrough]];
#endif
		case video_renderer::vulkan:
#if defined(HAVE_VULKAN)
			return video_renderer::vulkan;
#else
			[[fallthrough]];
#endif
		default:
		case video_renderer::null:
			return video_renderer::null;
		}
	}

	std::unique_ptr<render_backend> make_render_backend(video_renderer renderer, context* ctx, display_output* display)
	{
		switch (resolve_render_backend_type(renderer))
		{
#if !defined(__APPLE__)
		case video_renderer::opengl:
			return std::make_unique<GLGSRender>(ctx, display);
#endif
#if defined(HAVE_VULKAN)
		case video_renderer::vulkan:
			return std::make_unique<VKGSRender>(ctx, display);
#endif
		default:
			return std::make_unique<NullGSRender>(ctx, display);
		}
	}
}
