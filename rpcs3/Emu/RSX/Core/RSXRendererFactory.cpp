#include "stdafx.h"
#include "RSXRendererFactory.h"

#include "Emu/IdManager.h"
#include "Emu/RSX/Null/NullGSRender.h"

#ifndef __APPLE__
#include "Emu/RSX/GL/GLGSRender.h"
#endif

#if defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif

namespace rsx
{
	void create_render_thread(video_renderer renderer, utils::serial* ar)
	{
		switch (renderer)
		{
		case video_renderer::opengl:
#if not defined(__APPLE__)
			g_fxo->init<rsx::thread, named_thread<GLGSRender>>(ar);
#endif
			[[ fallthrough ]];
		case video_renderer::vulkan:
#if defined(HAVE_VULKAN)
			g_fxo->init<rsx::thread, named_thread<VKGSRender>>(ar);
#endif
			[[ fallthrough ]];
		default:
		case video_renderer::null:
			g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
		}
	}
}
