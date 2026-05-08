#include "stdafx.h"
#include "overlay_audio.h"
#include "Emu/System.h"

namespace rsx
{
	namespace overlays
	{
		audio_player::audio_player(const std::string& audio_path)
		{
			init_audio(audio_path);
		}

		void audio_player::init_audio(const std::string& audio_path)
		{
			if (audio_path.empty()) return;

			m_video_source = ensure(Emu.GetCallbacks().make_video_source());
			m_video_source->set_audio_path(audio_path);
		}

		void audio_player::set_active(bool active)
		{
			if (m_video_source)
			{
				m_video_source->set_active(active);
			}
		}
	}
}
