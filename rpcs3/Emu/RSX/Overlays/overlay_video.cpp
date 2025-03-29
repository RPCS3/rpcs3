#include "stdafx.h"
#include "overlay_video.h"
#include "Emu/System.h"

namespace rsx
{
	namespace overlays
	{
		video_view::video_view(const std::string& video_path)
		{
			m_video_source = Emu.GetCallbacks().make_video_source();
			ensure(!!m_video_source);

			m_video_source->set_video_path(video_path);

			// TODO: add timeout
			while (!m_video_source->has_new())
			{
				std::this_thread::sleep_for(1ms);
			}

			update();
		}

		video_view::~video_view()
		{
		}

		void video_view::update()
		{
			if (m_video_source->has_new())
			{
				auto& info = m_video_info.at(m_buffer_index);
				info = std::make_unique<video_info>();

				m_video_source->get_image(info->data, info->w, info->h, info->channels, info->bpp);
				set_raw_image(info.get());

				m_buffer_index = (m_buffer_index + 1) % m_video_info.size();
				is_compiled = false;
			}
		}

		compiled_resource& video_view::get_compiled()
		{
			update();

			return image_view::get_compiled();
		}
	}
}
