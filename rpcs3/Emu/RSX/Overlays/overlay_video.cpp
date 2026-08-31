#include "stdafx.h"
#include "overlay_video.h"
#include "Emu/System.h"
#include "Loader/ISO.h"

namespace rsx
{
	namespace overlays
	{
		video_view::video_view(const std::string& video_path, const std::string& audio_path, const std::string& thumbnail_path)
		{
			init_video(video_path, audio_path, false, false);

			if (!thumbnail_path.empty())
			{
				m_thumbnail_info = std::make_unique<image_info>(thumbnail_path);
				set_raw_image(m_thumbnail_info.get());
			}
		}

		video_view::video_view(const std::string& video_path, const std::string& audio_path, const std::vector<u8>& thumbnail_buf)
		{
			init_video(video_path, audio_path, false, false);

			if (!thumbnail_buf.empty())
			{
				m_thumbnail_info = std::make_unique<image_info>(thumbnail_buf);
				set_raw_image(m_thumbnail_info.get());
			}
		}

		video_view::video_view(const std::string& video_path, const std::string& audio_path, u8 thumbnail_id)
			: m_thumbnail_id(thumbnail_id)
		{
			init_video(video_path, audio_path, false, false);
			set_image_resource(thumbnail_id);
		}

		video_view::video_view(const GameInfo& info)
		{
			init_video(info.movie_path, info.audio_path, info.movie_in_archive, info.audio_in_archive);

			if (m_video_source && info.is_iso_file && (info.movie_in_archive || info.audio_in_archive) && is_iso_file(info.path))
			{
				m_video_source->set_iso_path(info.path);
			}

			if (auto img = image_info::load_icon(info.icon_path, info.icon_in_archive ? info.path : ""))
			{
				m_thumbnail_info = std::move(img);
				set_raw_image(m_thumbnail_info.get());
			}
		}

		video_view::~video_view()
		{
		}

		void video_view::init_video(const std::string& video_path, const std::string& audio_path, bool video_in_archive, bool audio_in_archive)
		{
			if (video_path.empty()) return;

			m_video_source = ensure(Emu.GetCallbacks().make_video_source());
			m_video_source->set_update_callback([this]()
			{
				if (m_video_active)
				{
					m_is_compiled = false;
				}
			});
			m_video_source->set_video_path(video_path, video_in_archive);
			m_video_source->set_audio_path(audio_path, audio_in_archive);
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
