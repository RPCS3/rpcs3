#include "stdafx.h"
#include "overlay_video.h"
#include "Emu/System.h"

namespace rsx
{
	namespace overlays
	{
		video_view::video_view(const std::string& video_path, const std::string& thumbnail_path)
		{
			init_video(video_path);

			if (!thumbnail_path.empty())
			{
				m_thumbnail_info = std::make_unique<image_info>(thumbnail_path);
				set_raw_image(m_thumbnail_info.get());
			}
		}

		video_view::video_view(const std::string& video_path, const std::vector<u8>& thumbnail_buf)
		{
			init_video(video_path);

			if (!thumbnail_buf.empty())
			{
				m_thumbnail_info = std::make_unique<image_info>(thumbnail_buf);
				set_raw_image(m_thumbnail_info.get());
			}
		}

		video_view::video_view(const std::string& video_path, u8 thumbnail_id)
			: m_thumbnail_id(thumbnail_id)
		{
			init_video(video_path);
			set_image_resource(thumbnail_id);
		}

		video_view::~video_view()
		{
		}

		void video_view::init_video(const std::string& video_path)
		{
			if (video_path.empty()) return;

			m_video_source = Emu.GetCallbacks().make_video_source();
			ensure(!!m_video_source);

			m_video_source->set_update_callback([this]()
			{
				if (m_video_active)
				{
					m_is_compiled = false;
				}
			});
			m_video_source->set_video_path(video_path);
		}

		void video_view::set_active(bool active)
		{
			if (m_video_source)
			{
				m_video_source->set_active(active);
				m_video_active = active;
				m_is_compiled = false;
			}
		}

		void video_view::update()
		{
			if (m_video_active && m_video_source && m_video_source->get_active())
			{
				if (!m_video_source->has_new())
				{
					return;
				}

				m_buffer_index = (m_buffer_index + 1) % m_video_info.size();

				auto& info = m_video_info.at(m_buffer_index);
				if (!info)
				{
					info = std::make_unique<video_info>();
				}

				m_video_source->get_image(info->data, info->w, info->h, info->channels, info->bpp);
				info->dirty = true;

				set_raw_image(info.get());
				m_is_compiled = false;
				return;
			}

			if (m_thumbnail_info && m_thumbnail_info.get() != external_ref)
			{
				set_raw_image(m_thumbnail_info.get());
				m_is_compiled = false;
				return;
			}

			if (m_thumbnail_id != image_resource_id::none && m_thumbnail_id != image_resource_ref)
			{
				set_image_resource(m_thumbnail_id);
				m_is_compiled = false;
				return;
			}
		}

		compiled_resource& video_view::get_compiled()
		{
			update();

			return external_ref ? image_view::get_compiled() : overlay_element::get_compiled();
		}
	}
}
